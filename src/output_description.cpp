
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
      const std::vector<Slice>& slices,
      const std::vector<Texture>& textures,
      const VariantMap& variables) {

    using TagKey = std::string;
    using TagValue = std::string;
    using SpriteIndex = int;
    using SliceIndex = int;
    using SourceIndex = int;
    auto tags = std::map<TagKey, std::map<TagValue, std::vector<SpriteIndex>>>();
    auto source_indices = std::map<ImagePtr, SourceIndex>();
    auto slice_sprites = std::map<SliceIndex, std::vector<SpriteIndex>>();
    auto source_sprites = std::map<SourceIndex, std::vector<SpriteIndex>>();
    auto sprite_on_slice = std::map<SpriteIndex, SliceIndex>();
    auto sprites_by_index = std::map<SpriteIndex, const Sprite*>();
    for (const auto& sprite : sprites)
      sprites_by_index[sprite.index] = &sprite;
    for (const auto& slice : slices)
      for (const auto& sprite : slice.sprites)
        sprite_on_slice[sprite.index] = slice.index;

    auto json = nlohmann::json{ };
    auto& json_sprites = json["sprites"];
    json_sprites = nlohmann::json::array();
    for (const auto& [sprite_index, sprite] : sprites_by_index) {
      auto& json_sprite = json_sprites.emplace_back();
      json_sprite["index"] = sprite_index;

      // output no more for dropped sprites
      if (!sprite->sheet)
        continue;

      const auto slice_index = sprite_on_slice.find(sprite_index)->second;
      const auto source_index = source_indices.emplace(
        sprite->source, to_int(source_indices.size())).first->second;

      json_sprite["id"] = sprite->id;
      json_sprite["inputSpriteIndex"] = sprite->input_sprite_index;
      json_sprite["rect"] = json_rect(sprite->rect);
      json_sprite["trimmedRect"] = json_rect(sprite->trimmed_rect);
      json_sprite["sourceIndex"] = source_index;
      json_sprite["sourceRect"] = json_rect(sprite->source_rect);
      json_sprite["trimmedSourceRect"] = json_rect(sprite->trimmed_source_rect);
      json_sprite["pivot"] = json_point(sprite->pivot);
      json_sprite["sliceIndex"] = slice_index;
      json_sprite["sliceSpriteIndex"] = slice_sprites[slice_index].size();
      json_sprite["rotated"] = sprite->rotated;
      json_sprite["tags"] = sprite->tags;
      json_sprite["data"] = json_variant_map(sprite->data);
      json_sprite["vertices"] = json_compact_point_list(sprite->vertices);

      for (const auto& [key, value] : sprite->tags)
        tags[key][value].push_back(sprite_index);
      source_sprites[source_index].push_back(sprite_index);
      slice_sprites[slice_index].push_back(sprite_index);
    }

    auto& json_tags = json["tags"];
    json_tags = nlohmann::json::object();
    for (const auto& [key, value_sprite_indices] : tags) {
      auto& json_tag = json_tags[key];
      for (const auto& [value, sprite_indices] : value_sprite_indices)
        json_tag[value] = sprite_indices;
    }

    auto& json_slices = json["slices"];
    json_slices = nlohmann::json::array();
    for (const auto& slice : slices) {
      auto& json_slice = json_slices.emplace_back();
      json_slice["index"] = slice.index;
      json_slice["inputFilename"] = path_to_utf8(slice.sheet->input_file);
      json_slice["spriteIndices"] = slice_sprites[slice.index];
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

    auto texture_index = size_t{ };
    auto& json_textures = json["textures"];
    json_textures = nlohmann::json::array();
    for (const auto& texture : textures) {
      if (texture.filename.empty())
        continue;

      auto& json_texture = json_textures.emplace_back();
      const auto& slice = *texture.slice;
      const auto& output = *texture.output;
      json_texture["index"] = texture_index++;
      json_texture["sliceIndex"] = texture.slice->index;
      json_texture["spriteIndices"] = slice_sprites[slice.index];
      json_texture["filename"] = texture.filename;
      json_texture["scale"] = output.scale;
      json_texture["width"] = to_int(slice.width * output.scale);
      json_texture["height"] = to_int(slice.height * output.scale);
      json_texture["map"] = (texture.map_index < 0 ?
        texture.output->default_map_suffix :
        texture.output->map_suffixes.at(to_unsigned(texture.map_index)));
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
    if (variable == "settings.rebuild")
      return to_string(settings.rebuild);
    if (auto it = variables.find(variable); it != variables.end())
      return variant_to_string(it->second);
    throw std::runtime_error("unknown id '" + std::string(variable) + "'");
  };

  const auto evaluate_sprite_expression = [&](const Sprite& sprite, 
      std::string& expression) {
    replace_variables(expression, [&](std::string_view variable) {
      if (variable == "index")
        return std::to_string(sprite.index);
      if (variable == "inputSpriteIndex")
        return std::to_string(sprite.input_sprite_index);
      if (variable == "sheet.id")
        return sprite.sheet->id;
      if (variable == "source.filename")
        return path_to_utf8(sprite.source->filename());
      return replace_variable(variable);
    });
  };

  const auto evaluate_slice_expression = [&](const Slice& slice, 
      std::string& expression) {
    replace_variables(expression, [&](std::string_view variable) {
      if (variable == "index")
        return std::to_string(slice.index);
      if (variable == "sheet.id")
        return slice.sheet->id;
      if (variable == "sprite.id")
        return (slice.sprites.empty() ? "" : slice.sprites[0].id);
      return replace_variable(variable);
    });
  };

  for (auto& sprite : sprites)
    evaluate_sprite_expression(sprite, sprite.id);

  for (auto& texture : textures)
    evaluate_slice_expression(*texture.slice, texture.filename);
}

std::string get_description(const std::string& template_source,
    const std::vector<Sprite>& sprites, 
    const std::vector<Slice>& slices) {
  auto ss = std::stringstream();
  const auto json = get_json_description(sprites, slices, { }, { });
  auto env = setup_inja_environment(&json);
  env.render_to(ss, env.parse(template_source), json);
  return ss.str();
}

bool output_description(const Settings& settings,
    const std::vector<Sprite>& sprites, 
    const std::vector<Slice>& slices,
    const std::vector<Texture>& textures,
    const VariantMap& variables) try {
  auto ss = std::stringstream();
  auto& os = (settings.output_file == "stdout" ? std::cout : ss);

  const auto json = get_json_description(sprites, slices, textures, variables);
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
