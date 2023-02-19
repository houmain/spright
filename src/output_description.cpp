
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

  nlohmann::json json_compact_point_list(const std::vector<PointF>& points) {
    auto list = nlohmann::json::array();
    for (const auto& point : points) {
      list.push_back(point.x);
      list.push_back(point.y);
    }
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
    using SpriteIndex = int;
    using TextureIndex = int;
    using SourceIndex = int;
    auto tags = std::map<TagKey, std::map<TagValue, std::vector<SpriteIndex>>>();
    auto source_indices = std::map<ImagePtr, SourceIndex>();
    auto texture_sprites = std::map<TextureIndex, std::vector<SpriteIndex>>();
    auto source_sprites = std::map<SourceIndex, std::vector<SpriteIndex>>();
    auto sprite_on_texture = std::map<SpriteIndex, TextureIndex>();
    auto sprites_by_index = std::map<SpriteIndex, const Sprite*>();
    for (const auto& sprite : sprites)
      sprites_by_index[sprite.index] = &sprite;
    for (const auto& texture : textures)
      for (const auto& sprite : texture.sprites)
        sprite_on_texture[sprite.index] = texture.index;

    auto json = nlohmann::json{ };
    auto& json_sprites = json["sprites"];
    json_sprites = nlohmann::json::array();
    for (const auto& [sprite_index, sprite] : sprites_by_index) {
      auto& json_sprite = json_sprites.emplace_back();
      json_sprite["index"] = sprite_index;

      // output no more for dropped sprites
      if (!sprite->sheet)
        continue;

      const auto texture_index = sprite_on_texture.find(sprite_index)->second;
      const auto source_index = source_indices.emplace(
        sprite->source, to_int(source_indices.size())).first->second;

      json_sprite["id"] = sprite->id;
      json_sprite["inputSpriteIndex"] = sprite->input_sprite_index;
      json_sprite["rect"] = json_rect(sprite->rect);
      json_sprite["trimmedRect"] = json_rect(sprite->trimmed_rect);
      json_sprite["sourceIndex"] = source_index;
      json_sprite["sourceRect"] = json_rect(sprite->source_rect);
      json_sprite["trimmedSourceRect"] = json_rect(sprite->trimmed_source_rect);
      json_sprite["pivot"] = json_point(sprite->pivot_point);
      json_sprite["textureIndex"] = texture_index;
      json_sprite["textureSpriteIndex"] = texture_sprites[texture_index].size();
      json_sprite["rotated"] = sprite->rotated;
      json_sprite["tags"] = sprite->tags;
      json_sprite["data"] = json_variant_map(sprite->data);
      json_sprite["vertices"] = json_compact_point_list(sprite->vertices);

      for (const auto& [key, value] : sprite->tags)
        tags[key][value].push_back(sprite_index);
      source_sprites[source_index].push_back(sprite_index);
      texture_sprites[texture_index].push_back(sprite_index);
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
    for (const auto& texture : textures) {
      auto& json_texture = json_textures.emplace_back();
      json_texture["index"] = texture.index;
      json_texture["inputFilename"] = path_to_utf8(texture.sheet->input_file);
      json_texture["filename"] = texture.filename;
      json_texture["width"] = texture.width;
      json_texture["height"] = texture.height;
      json_texture["spriteIndices"] = texture_sprites[texture.index];
    }

    auto& json_sources = json["sources"];
    json_sources = nlohmann::json::array();
    auto sources_by_index = std::map<SourceIndex, ImagePtr>();
    for (const auto& [source, index] : source_indices)
      sources_by_index[index] = source;
    for (const auto& [index, source] : sources_by_index) {
      auto& json_source = json_sources.emplace_back();
      json_source["index"] = index;
      json_source["path"] = path_to_utf8(source->path());
      json_source["filename"] = path_to_utf8(source->filename());
      json_source["width"] = source->width();
      json_source["height"] = source->height();
      json_source["spriteIndices"] = source_sprites[index];
    }

    for (const auto& [key, value] : variables)
      std::visit([&, k = &key](const auto& v) { json[*k] = v; }, value);

    return json;
  }

  inja::Environment setup_inja_environment([[maybe_unused]] const nlohmann::json* json) {
    auto env = inja::Environment();
    env.set_trim_blocks(false);
    env.set_lstrip_blocks(false);

    env.add_callback("removeExtension", 1, [](inja::Arguments& args) -> inja::json {
      return remove_extension(args.at(0)->get<std::string>());
    });
    return env;
  }

  template<typename F>
  void replace_variables(std::string& expression, F&& replace_function) {
    for (;;) {
      const auto begin = expression.find("{{");
      if (begin == std::string::npos)
        break;
      const auto end = expression.find("}}", begin);
      if (end == std::string::npos)
        break;
      const auto variable = trim(std::string_view{
          expression.data() + begin + 2, end - begin - 2 });
      expression.replace(begin, end + 2, replace_function(variable));
    }
  }

  std::string variant_to_string(const Variant& variant) {
    auto string = std::string();
    std::visit([&](const auto& value) {
      using T = std::decay_t<decltype(value)>;
      if constexpr (std::is_same_v<T, std::string>) {
        string = value;
      }
      else {
        using namespace std;
        string = to_string(value);
      }
    }, variant);
    return string;
  }
} // namespace

void evaluate_expressions(const Settings& settings,
    std::vector<Sprite>& sprites,
    std::vector<Texture>& textures,
    VariantMap& variables) {

  const auto replace_variable = [&](std::string_view variable) {
    if (variable == "settings.autocomplete")
      return to_string(settings.autocomplete);
    if (variable == "settings.debug")
      return to_string(settings.debug);
    if (variable == "settings.rebuild")
      return to_string(settings.rebuild);
    if (auto it = variables.find(variable); it != variables.end())
      return variant_to_string(it->second);
    throw std::runtime_error("unknown id '" + std::string(variable) + "'");
  };

  const auto evaluate_sprite_expression = [&](Sprite& sprite, std::string& expression) {
    replace_variables(expression, [&](std::string_view variable) {
      if (variable == "index")
        return std::to_string(sprite.index);
      if (variable == "inputSpriteIndex")
        return std::to_string(sprite.input_sprite_index);
      if (variable == "source.filename")
        return path_to_utf8(sprite.source->filename());
      return replace_variable(variable);
    });
  };

  const auto evaluate_texture_expression = [&](Texture& texture, std::string& expression) {
    replace_variables(expression, [&](std::string_view variable) {
      if (variable == "index")
        return std::to_string(texture.index);
      if (variable == "sprite.id")
        return (texture.sprites.empty() ? "" : texture.sprites[0].id);
      return replace_variable(variable);
    });
  };

  for (auto& sprite : sprites)
    evaluate_sprite_expression(sprite, sprite.id);

  for (auto& texture : textures)
    evaluate_texture_expression(texture, texture.filename);
}

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
    const VariantMap& variables) try {
  auto ss = std::stringstream();
  auto& os = (settings.output_file == "stdout" ? std::cout : ss);

  const auto json = get_json_description(sprites, textures, variables);
  if (!settings.template_file.empty()) {
    auto env = setup_inja_environment(&json);
    env.render_to(os, env.parse_template(path_to_utf8(settings.template_file)), json);
  }
  else {
    os << json.dump(1, '\t');
  }
  if (settings.output_file != "stdout")
    if (!update_textfile(settings.output_path / settings.output_file, ss.str()))
      return false;

  return true;
}
catch (const std::exception& ex) {
  throw std::runtime_error(
    (settings.template_file.empty() ? "" :
     "'" + path_to_utf8(settings.template_file) + "'") + ex.what());
}

} // namespace
