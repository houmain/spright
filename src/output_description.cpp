
#include "output.h"
#include "inja/inja.hpp"
#include <fstream>

namespace spright {

namespace {
  nlohmann::json json_point(const PointF& point) {
    auto json_point = nlohmann::json::object();
    json_point["x"] = point.x;
    json_point["y"] = point.y;
    return json_point;
  }

  nlohmann::json json_point_list(const std::vector<PointF>& points) {
    auto list = nlohmann::json::array();
    for (const auto& point : points)
      list.push_back(json_point(point));
    return list;
  }

  nlohmann::json json_rect(const Rect& rect) {
    auto json_rect = nlohmann::json::object();
    json_rect["x"] = rect.x;
    json_rect["y"] = rect.y;
    json_rect["w"] = rect.w;
    json_rect["h"] = rect.h;
    return json_rect;
  }

  nlohmann::json json_variant_map(const VariantMap& map) {
    auto json_map = nlohmann::json::object();
    for (const auto& [key, value] : map)
      std::visit([&, k = &key](const auto& v) { 
        json_map[*k] = v;
      }, value);
    return json_map;
  }

  nlohmann::json get_json_description(
      const std::vector<Sprite>& sprites,
      const std::vector<Texture>& textures,
      const VariantMap& variables) {

    using TagKey = std::string;
    using TagValue = std::string;
    using SpriteIndex = size_t;
    using TextureIndex = size_t;
    using SourceIndex = size_t;
    auto tags = std::map<TagKey, std::map<TagValue, std::vector<SpriteIndex>>>();
    auto texture_indices = std::map<std::string, TextureIndex>();
    auto source_indices = std::map<ImagePtr, SourceIndex>();
    auto texture_sprites = std::map<TextureIndex, std::vector<SpriteIndex>>();
    auto source_sprites = std::map<SourceIndex, std::vector<SpriteIndex>>();

    auto json = nlohmann::json{ };
    for (const auto& [key, value] : variables)
      std::visit([&, k = &key](const auto& v) { json[*k] = v; }, value);

    auto& json_sprites = json["sprites"];
    json_sprites = nlohmann::json::array();

    for (const auto& sprite : sprites) {
      if (!sprite.output || !sprite.source)
        continue;

      auto& json_sprite = json_sprites.emplace_back();
      const auto index = json_sprites.size() - 1;
      const auto texture_filename = 
        sprite.output->filename.get_nth_filename(sprite.texture_filename_index);
      const auto texture_index = texture_indices.emplace(
        texture_filename, texture_indices.size()).first->second;
      const auto source_index = source_indices.emplace(
        sprite.source, source_indices.size()).first->second;

      json_sprite["index"] = sprite.index;
      json_sprite["id"] = sprite.id;
      json_sprite["rect"] = json_rect(sprite.rect);
      json_sprite["trimmedRect"] = json_rect(sprite.trimmed_rect);
      json_sprite["sourceIndex"] = source_index;
      json_sprite["sourceRect"] = json_rect(sprite.source_rect);
      json_sprite["trimmedSourceRect"] = json_rect(sprite.trimmed_source_rect);
      json_sprite["pivot"] = json_point(sprite.pivot_point);
      json_sprite["textureIndex"] = texture_index;
      json_sprite["textureSpriteIndex"] = texture_sprites[texture_index].size();
      json_sprite["rotated"] = sprite.rotated;
      json_sprite["tags"] = sprite.tags;
      json_sprite["data"] = json_variant_map(sprite.data);
      json_sprite["vertices"] = json_point_list(sprite.vertices);

      for (const auto& [key, value] : sprite.tags)
        tags[key][value].push_back(index);
      source_sprites[source_index].push_back(index);
      texture_sprites[texture_index].push_back(index);
    }

    auto& json_tags = json["tags"];
    json_tags = nlohmann::json::object();
    for (const auto& [key, value_sprite_indices] : tags) {
      auto& json_tag = json_tags[key];
      for (const auto& [value, sprite_indices] : value_sprite_indices)
        json_tag[value] = sprite_indices;
    }

    auto& json_textures = json["textures"];
    json_textures = nlohmann::json::array();
    for (auto i = TextureIndex{ }; i < textures.size(); ++i) {
      const auto& texture = textures[i];
      auto& json_texture = json_textures.emplace_back();
      const auto filename = 
        texture.output->filename.get_nth_filename(texture.filename_index);
      json_texture["inputFilename"] = path_to_utf8(texture.output->input_file);
      json_texture["filename"] = filename;
      json_texture["width"] = texture.width;
      json_texture["height"] = texture.height;
      json_texture["spriteIndices"] = texture_sprites[i];
    }

    auto& json_sources = json["sources"];
    json_sources = nlohmann::json::array();
    auto sources_by_index = std::map<SourceIndex, ImagePtr>();
    for (const auto& [source, index] : source_indices)
      sources_by_index[index] = source;
    for (const auto& [index, source] : sources_by_index) {
      auto& json_source = json_sources.emplace_back();
      json_source["path"] = path_to_utf8(source->path());
      json_source["filename"] = path_to_utf8(source->filename());
      json_source["width"] = source->width();
      json_source["height"] = source->height();
      json_source["spriteIndices"] = source_sprites[index];
    }
    return json;
  }

  std::string remove_extension(std::string filename) {
    const auto dot = filename.rfind('.');
    if (dot != std::string::npos)
      filename.resize(dot);
    return filename;
  }

  std::string generate_sprite_id(int index) {
    return "sprite_" + std::to_string(index);
  }

  inja::Environment setup_inja_environment(const nlohmann::json* json) {
    auto env = inja::Environment();
    env.set_trim_blocks(false);
    env.set_lstrip_blocks(false);

    env.add_callback("getId", 1, [](inja::Arguments& args) -> inja::json {
      const auto sprite = args.at(0)->get<inja::json>();
      const auto id = std::string{ sprite["id"] };
      return (!id.empty() ? id : generate_sprite_id(sprite["index"]));
    });

    env.add_callback("getIdOrFilename", 1, [json](inja::Arguments& args) -> inja::json {
      const auto sprite = args.at(0)->get<inja::json>();
      const auto id = std::string{ sprite["id"] };
      if (!id.empty())
        return id;
      const auto source_index = size_t{ sprite["sourceIndex"] };
      return (*json)["sources"][source_index]["filename"];
    });

    env.add_callback("removeExtension", 1, [](inja::Arguments& args) -> inja::json {
      return remove_extension(args.at(0)->get<std::string>());
    });
    return env;
  }
} // namespace

std::string get_description(const std::string& template_source,
    const std::vector<Sprite>& sprites, 
    const std::vector<Texture>& textures) {
  auto ss = std::stringstream();
  const auto json = get_json_description(sprites, textures, { });
  auto env = setup_inja_environment(&json);
  env.render_to(ss, env.parse(template_source), json);
  return ss.str();
}

bool write_output_description(const Settings& settings,
    const std::vector<Sprite>& sprites, 
    const std::vector<Texture>& textures,
    const VariantMap& variables) {
  auto ss = std::stringstream();
  auto& os = (settings.output_file == "stdout" ? std::cout : ss);

  const auto json = get_json_description(sprites, textures, variables);
  if (!settings.template_file.empty()) {
    auto env = setup_inja_environment(&json);
    env.render_to(os, env.parse_template(path_to_utf8(settings.template_file)), json);
  }
  else {
    os << json.dump(2);
  }
  if (settings.output_file != "stdout")
    if (!update_textfile(settings.output_path / settings.output_file, ss.str()))
      return false;

  return true;
}

} // namespace
