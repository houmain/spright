
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
      const Settings& settings,
      const std::vector<Input>& inputs, 
      const std::vector<Sprite>& sprites,
      const std::vector<Slice>& slices,
      const std::vector<Texture>& textures,
      const VariantMap& variables) {

    using TagKey = std::string;
    using TagValue = std::string;
    using InputIndex = int;
    using SpriteIndex = int;
    using SliceIndex = int;
    using SourceIndex = int;
    auto tags = std::map<TagKey, std::map<TagValue, std::vector<SpriteIndex>>>();
    auto source_indices = std::map<ImagePtr, SourceIndex>();
    auto slice_sprites = std::map<SliceIndex, std::vector<SpriteIndex>>();
    auto sprite_on_slice = std::map<SpriteIndex, SliceIndex>();
    auto sprites_by_index = std::map<SpriteIndex, const Sprite*>();
    auto input_source_sprites = std::map<std::pair<InputIndex, SourceIndex>, std::vector<SpriteIndex>>();
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

      const auto source_index = source_indices.emplace(
        sprite->source, to_int(source_indices.size())).first->second;

      json_sprite["id"] = sprite->id;
      json_sprite["inputIndex"] = sprite->input_index;
      json_sprite["inputSpriteIndex"] = sprite->input_sprite_index;
      json_sprite["sourceIndex"] = source_index;
      json_sprite["sourceRect"] = json_rect(sprite->source_rect);
      json_sprite["tags"] = sprite->tags;
      json_sprite["data"] = json_variant_map(sprite->data);
      input_source_sprites[{ sprite->input_index, source_index }].push_back(sprite_index);

      for (const auto& [key, value] : sprite->tags)
        tags[key][value].push_back(sprite_index);

      // only available when packing was executed
      if (sprite->slice_index >= 0) {
        auto slice_index = sprite->slice_index;
        if (const auto it = sprite_on_slice.find(sprite_index); it != sprite_on_slice.end())
          slice_index = it->second;
        json_sprite["sliceIndex"] = slice_index;
        json_sprite["sliceSpriteIndex"] = slice_sprites[slice_index].size();
        json_sprite["rect"] = json_rect(sprite->rect);
        json_sprite["trimmedRect"] = json_rect(sprite->trimmed_rect);
        json_sprite["trimmedSourceRect"] = json_rect(sprite->trimmed_source_rect);
        json_sprite["pivot"] = json_point(sprite->pivot);
        json_sprite["rotated"] = sprite->rotated;
        json_sprite["vertices"] = json_compact_point_list(sprite->vertices);
        slice_sprites[slice_index].push_back(sprite_index);
      }
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
      json_slice["spriteIndices"] = slice_sprites[slice.index];
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
    }

    auto& json_inputs = json["inputs"];
    json_inputs = nlohmann::json::array();
    for (const auto& input : inputs) {
      auto& json_input = json_inputs.emplace_back();
      json_input["filename"] = input.source_filenames;
      auto json_sources = nlohmann::json::array();
      for (const auto& source : input.sources) {
        auto& json_source = json_sources.emplace_back();
        const auto source_index = source_indices[source];
        json_source["index"] = source_index;
        json_source["spriteIndices"] = input_source_sprites[{ input.index, source_index }];
      }
      json_input["sources"] = std::move(json_sources);
    }

    auto& json_textures = json["textures"];
    json_textures = nlohmann::json::array();
    for (const auto& texture : textures) {
      if (texture.filename.empty())
        continue;

      auto& json_texture = json_textures.emplace_back();
      const auto& slice = *texture.slice;
      const auto& output = *texture.output;
      json_texture["sliceIndex"] = texture.slice->index;
      json_texture["spriteIndices"] = slice_sprites[slice.index];
      json_texture["path"] = path_to_utf8(settings.output_path);
      json_texture["filename"] = path_to_utf8(
        settings.output_path.empty() ? texture.filename :
          std::filesystem::relative(texture.filename, settings.output_path));
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

  inja::Environment setup_inja_environment() {
    auto env = inja::Environment();
    env.set_trim_blocks(false);
    env.set_lstrip_blocks(false);

    env.add_callback("removeExtension", 1, [](inja::Arguments& args) -> inja::json {
      return remove_extension(args.at(0)->get<std::string>());
    });
    env.add_callback("removeDirectories", [](inja::Arguments& args) -> inja::json {
      return remove_directory(
        (args.size() > 0 ? args[0]->get<std::string>() : ""),
        (args.size() > 1 ? args[1]->get<int>() : 0));
    });
    env.add_callback("joinPaths", [](inja::Arguments& args) -> inja::json {
      auto path = std::filesystem::path();
      for (auto i = 0u; i < args.size(); ++i)
        if (const auto dir = args[i]->get<std::string>(); !dir.empty())
          path /= utf8_to_path(dir);
      return path_to_utf8(path);
    });
    env.add_callback("floor", 1, [](inja::Arguments& args) -> inja::json {
      return static_cast<int>(std::floor(args.at(0)->get<real>()));
    });
    env.add_callback("ceil", 1, [](inja::Arguments& args) -> inja::json {
      return static_cast<int>(std::ceil(args.at(0)->get<real>()));
    });
    env.add_callback("makeId", 1, [](inja::Arguments& args) -> inja::json {
      return make_identifier(args.at(0)->get<std::string>());
    });
    env.add_callback("base64", 1, [](inja::Arguments& args) -> inja::json {
      return base64_encode_file(args.at(0)->get<std::string>());
    });
    return env;
  }

  void output_description(std::ostream& os,
      const std::filesystem::path& template_filename,
      const nlohmann::json& json) {
    if (!template_filename.empty()) {
      auto env = setup_inja_environment();
      env.render_to(os, env.parse_template(
        path_to_utf8(template_filename)), json);
    }
    else {
      os << json.dump(1, '\t');
    }
  }

  auto filter_by_slice(int slice_index,
      const Slice& sole_slice,
      const std::vector<Sprite>& sprites, 
      const std::vector<Texture>& textures) ->
      std::tuple<std::vector<Sprite>, std::vector<Texture>> {
    
    auto slice_sprites = std::vector<Sprite>();
    std::copy_if(sprites.begin(), sprites.end(),
      std::back_inserter(slice_sprites),
      [&](const Sprite& sprite) {
        return (sprite.slice_index == slice_index);
      });

    auto index = 0;
    for (auto& sprite : slice_sprites) {
      sprite.index = index++;
      sprite.slice_index = 0;
    }

    auto slice_textures = std::vector<Texture>();    
    std::copy_if(textures.begin(), textures.end(),
      std::back_inserter(slice_textures),
      [&](const Texture& texture) {
        return (texture.slice->index == slice_index);
      });

    for (auto& texture : slice_textures)
      texture.slice = &sole_slice;

    return { slice_sprites, slice_textures };
  }
} // namespace

void evaluate_expressions(
    [[maybe_unused]] const Settings& settings,
    std::vector<Sprite>& sprites,
    std::vector<Texture>& textures,
    VariantMap& variables) {

  const auto replace_variable = [&](std::string_view variable) {
    if (auto it = variables.find(variable); it != variables.end())
      return variant_to_string(it->second);
    error("unknown id '", variable, "'");
  };

  const auto evaluate_sprite_expression = [&](const Sprite& sprite, 
      std::string& expression) {
    replace_variables(expression, [&](std::string_view variable) {
      if (variable == "index")
        return std::to_string(sprite.index);
      if (variable == "inputIndex")
        return std::to_string(sprite.input_index);
      if (variable == "inputSpriteIndex")
        return std::to_string(sprite.input_sprite_index);
      if (variable == "sheet.id")
        return sprite.sheet->id;

      // "dir/file 01.png"
      if (variable == "source.filename")
        return path_to_utf8(sprite.source->filename());

      // "dir/file 01"
      if (variable == "source.filenameBase")
        return remove_extension(path_to_utf8(sprite.source->filename()));

      // "file 01"
      if (variable == "source.filenameStem")
        return remove_extension(
          path_to_utf8(sprite.source->filename().filename()));

      // "dir_file_01"
      if (variable == "source.filenameId")
        return make_identifier(remove_extension(
          path_to_utf8(sprite.source->filename())));

      // "dir"
      if (variable == "source.dirname")
        return path_to_utf8(sprite.source->filename().parent_path());

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
    try {
      evaluate_sprite_expression(sprite, sprite.id);
      for (auto& [key, value] : sprite.tags)
        evaluate_sprite_expression(sprite, value);
    }
    catch (const std::exception& ex) {
      warning(ex.what(), sprite.warning_line_number);
    }

  for (auto& texture : textures)
    try {
      auto filename = path_to_utf8(texture.filename);
      evaluate_slice_expression(*texture.slice, filename);
      texture.filename = utf8_to_path(filename);
    }
    catch (const std::exception& ex) {
      warning(ex.what(), texture.output->warning_line_number);
    }
}

void resolve_template_filename(std::filesystem::path& template_filename) {
  auto error_code = std::error_code{ };
  for (const auto& prefix : { ".", "./templates", "../share/spright/templates" }) {
    const auto resolved = prefix / template_filename;
    if (std::filesystem::exists(resolved, error_code)) {
      template_filename = resolved;
      return;
    }
  }
  error("template '", path_to_utf8(template_filename), "' not found");
}

void complete_description_definitions(const Settings& settings,
    std::vector<Description>& descriptions,
    const VariantMap& variables) {

  // ignore description in definition
  if (settings.mode == Mode::describe ||
      settings.mode == Mode::describe_input)
    descriptions.clear();

  // use description filename/template from settings
  if (descriptions.empty() ||
      (settings.output_file_set || !settings.template_file.empty())) {
    auto& description = descriptions.emplace_back();
    description.filename = settings.output_file;
    description.template_filename = settings.template_file;
  }
  
  // prepend output path
  if (!settings.output_path.empty())
    for (auto& description : descriptions)
      if (description.filename.string() != "stdout")
        description.filename = settings.output_path / description.filename;

  // replace variables and resolve template filename
  for (auto& description : descriptions) {
    replace_variables(description.template_filename, variables);
    resolve_template_filename(description.template_filename);
  }
}

std::string dump_description(
    const std::string& template_source,
    const std::vector<Sprite>& sprites, 
    const std::vector<Slice>& slices) {
  auto ss = std::ostringstream();
  const auto json = get_json_description({ }, { }, sprites, slices, { }, { });
  auto env = setup_inja_environment();
  env.render_to(ss, env.parse(template_source), json);
  return ss.str();
}

void output_descriptions(
    const Settings& settings,
    const std::vector<Description>& descriptions, 
    const std::vector<Input>& inputs, 
    const std::vector<Sprite>& sprites, 
    const std::vector<Slice>& slices,
    const std::vector<Texture>& textures,
    const VariantMap& variables) {

  auto json = std::optional<nlohmann::json>();

  for (const auto& description : descriptions) {
    if (description.filename.empty())
      continue;

    const auto filenames = FilenameSequence(path_to_utf8(description.filename));
    if (!filenames.is_sequence()) {
      // output all slices in one output description
      if (!json.has_value())
        json = get_json_description(settings,
          inputs, sprites, slices, textures, variables);

      if (description.filename.string() != "stdout") {
        auto ss = std::ostringstream();
        output_description(ss, description.template_filename, *json);
        update_textfile(description.filename, ss.str());
      }
      else {
        output_description(std::cout, description.template_filename, *json);
      }
    }
    else {
      // output each slice in separate output description
      for (const auto& slice : slices) {
        auto sole_slice = slice;
        sole_slice.index = 0;

        const auto [slice_sprites, slice_textures] =
          filter_by_slice(slice.index, sole_slice, sprites, textures);
        
        const auto slice_json = get_json_description(settings,
          inputs, slice_sprites, { sole_slice }, slice_textures, variables);

        const auto filename = filenames.get_nth_filename(slice.index);
        auto ss = std::ostringstream();
        output_description(ss, description.template_filename, slice_json);
        update_textfile(filename, ss.str());
      }
    }
  }
}

} // namespace
