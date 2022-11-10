
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
  };

  nlohmann::json get_json_description(const std::vector<Sprite>& sprites,
      const std::vector<Texture>& textures) {
    auto json = nlohmann::json{ };
    auto& json_sprites = json["sprites"];
    json_sprites = nlohmann::json::array();

    using TagKey = std::pair<std::string, std::string>;
    using SpriteIndex = size_t;
    auto tags = std::map<TagKey, std::vector<SpriteIndex>>();
    auto texture_sprites = std::map<std::string, std::vector<SpriteIndex>>();

    for (const auto& sprite : sprites) {
      if (!sprite.output || !sprite.source)
        continue;

      auto& json_sprite = json_sprites.emplace_back();
      const auto index = json_sprites.size() - 1;
      const auto texture_filename = 
        sprite.output->filename.get_nth_filename(sprite.texture_index);
      json_sprite["index"] = sprite.index;
      json_sprite["id"] = sprite.id;
      json_sprite["rect"] = json_rect(sprite.rect);
      json_sprite["trimmedRect"] = json_rect(sprite.trimmed_rect);
      json_sprite["sourceFilename"] = path_to_utf8(sprite.source->filename());
      json_sprite["sourcePath"] = path_to_utf8(sprite.source->path());
      json_sprite["sourceRect"] = json_rect(sprite.source_rect);
      json_sprite["trimmedSourceRect"] = json_rect(sprite.trimmed_source_rect);
      json_sprite["pivot"] = json_point(sprite.pivot_point);
      json_sprite["textureFilename"] = texture_filename;
      json_sprite["textureSpriteIndex"] = texture_sprites[texture_filename].size();
      json_sprite["rotated"] = sprite.rotated;
      json_sprite["tags"] = sprite.tags;
      json_sprite["vertices"] = json_point_list(sprite.vertices);
      for (const auto& tag_key : sprite.tags)
        tags[tag_key].push_back(index);
      texture_sprites[texture_filename].push_back(index);
    }

    auto& json_tags = json["tags"];
    json_tags = nlohmann::json::array();
    for (const auto& [tag_key, sprite_indices] : tags) {
      auto& json_tag = json_tags.emplace_back();
      json_tag["key"] = tag_key.first;
      if (!tag_key.second.empty())
        json_tag["value"] = tag_key.second;
      json_tag["spriteIndices"] = sprite_indices;
    }

    auto& json_textures = json["textures"];
    json_textures = nlohmann::json::array();
    for (const auto& texture : textures) {
      auto& json_texture = json_textures.emplace_back();
      const auto filename = 
        texture.output->filename.get_nth_filename(texture.index);
      json_texture["filename"] = filename;
      json_texture["width"] = texture.width;
      json_texture["height"] = texture.height;
      json_texture["spriteIndices"] = texture_sprites[filename];
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

  inja::Environment setup_inja_environment() {
    auto env = inja::Environment();
    env.set_trim_blocks(false);
    env.set_lstrip_blocks(false);

    env.add_callback("getId", 1, [](inja::Arguments& args) -> inja::json {
      const auto& s = args.at(0)->get<inja::json>();
      const auto id = std::string(s["id"]);
      return (!id.empty() ? id : generate_sprite_id(s["index"]));
    });
    env.add_callback("getIdOrFilename", 1, [](inja::Arguments& args) -> inja::json {
      const auto& s = args.at(0)->get<inja::json>();
      const auto id = std::string(s["id"]);
      return (!id.empty() ? id : std::string(s["sourceFilename"]));
    });
    env.add_callback("removeExtension", 1, [](inja::Arguments& args) -> inja::json {
      return remove_extension(args.at(0)->get<std::string>());
    });
    return env;
  }

  const Image* get_source(const Sprite& sprite, int layer_index) {
    if (layer_index < 0)
      return sprite.source.get();
    const auto index = static_cast<size_t>(layer_index);
    if (sprite.layers && index < sprite.layers->size())
      return sprite.layers->at(index).get();
    return nullptr;
  }

  bool copy_sprite(Image& target, const Sprite& sprite, int layer_index) try {
    const auto source = get_source(sprite, layer_index);
    if (!source)
      return false;

    if (sprite.rotated) {
      if (sprite.vertices.empty()) {
        copy_rect_rotated_cw(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
      }
      else {
        copy_rect_rotated_cw(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y, sprite.vertices);
      }
    }
    else {
      if (sprite.vertices.empty()) {
        copy_rect(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
      }
      else {
        copy_rect(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y, sprite.vertices);
      }
    }

    if (sprite.extrude) {
      const auto left = (sprite.source_rect.x0() == sprite.trimmed_source_rect.x0());
      const auto top = (sprite.source_rect.y0() == sprite.trimmed_source_rect.y0());
      const auto right = (sprite.source_rect.x1() == sprite.trimmed_source_rect.x1());
      const auto bottom = (sprite.source_rect.y1() == sprite.trimmed_source_rect.y1());
      if (left || top || right || bottom) {
        auto rect = sprite.trimmed_rect;
        if (sprite.rotated)
          std::swap(rect.w, rect.h);
        for (auto i = 0; i < sprite.extrude; i++) {
          rect = expand(rect, 1);
          extrude_rect(target, rect, left, top, right, bottom);
        }
      }
    }
    return true;
  }
  catch (const std::exception& ex) {
#if defined(NDEBUG)
    throw;
#else
    std::fprintf(stderr, "copying sprite failed: %s\n", ex.what());
#endif
    return false;
  }

  void process_alpha(Image& target, const Texture& texture) {
    switch (texture.output->alpha) {
      case Alpha::keep:
        break;

      case Alpha::clear:
        clear_alpha(target);
        break;

      case Alpha::bleed:
        bleed_alpha(target);
        break;

      case Alpha::premultiply:
        premultiply_alpha(target);
        break;

      case Alpha::colorkey:
        make_opaque(target, texture.output->colorkey);
        break;
    }
  }

  void draw_debug_info(Image& target, const Sprite& sprite) {
    auto rect = sprite.rect;
    auto trimmed_rect = sprite.trimmed_rect;
    auto pivot_point = sprite.pivot_point;
    if (sprite.rotated) {
      std::swap(rect.w, rect.h);
      std::swap(trimmed_rect.w, trimmed_rect.h);
      pivot_point = rotate_cw(pivot_point, rect.w - 1);
    }
    const auto pivot_rect = Rect{
      rect.x + static_cast<int>(pivot_point.x - 0.25f),
      rect.y + static_cast<int>(pivot_point.y - 0.25f),
      (pivot_point.x == std::floor(pivot_point.x) ? 2 : 1),
      (pivot_point.y == std::floor(pivot_point.y) ? 2 : 1),
    };
    draw_rect(target, rect, RGBA{ { 255, 0, 255, 128 } });
    draw_rect(target, trimmed_rect, RGBA{ { 255, 255, 0, 128 } });
    draw_rect(target, pivot_rect, RGBA{ { 255, 0, 0, 255 } });
    // draw_rect(target, expand(rect, -1), RGBA{ { 255, 255, 0, 128 } });
    // draw_rect(target, expand(pivot_rect, 1), RGBA{ { 255, 255, 0, 128 } });

    if (!sprite.vertices.empty()) {
      const auto x = static_cast<float>(sprite.trimmed_rect.x);
      const auto y = static_cast<float>(sprite.trimmed_rect.y);
      for (auto i = 0u; i < sprite.vertices.size(); i++) {
        auto v0 = sprite.vertices[i];
        auto v1 = sprite.vertices[(i + 1) % sprite.vertices.size()];
        if (sprite.rotated) {
          v0 = rotate_cw(v0, trimmed_rect.w);
          v1 = rotate_cw(v1, trimmed_rect.w);
        }
        draw_line(target,
          static_cast<int>(x + v0.x),
          static_cast<int>(y + v0.y),
          static_cast<int>(x + v1.x),
          static_cast<int>(y + v1.y),
          RGBA{ { 0, 255, 255, 128 } },
          true);
      }
    }
  }
} // namespace

std::string get_description(const std::string& template_source,
    const std::vector<Sprite>& sprites, const std::vector<Texture>& textures) {
  auto ss = std::stringstream();
  const auto json = get_json_description(sprites, textures);
  auto env = setup_inja_environment();
  env.render_to(ss, env.parse(template_source), json);
  return ss.str();
}

void write_output_description(const Settings& settings,
    const std::vector<Sprite>& sprites, const std::vector<Texture>& textures) {
  if (settings.output_file.empty())
    return;

  auto file = std::ofstream();
  auto& os = [&]() -> std::ostream& {
    if (settings.output_file == "stdout")
      return std::cout;
    const auto filename = settings.output_path / settings.output_file;
    auto error = std::error_code{ };
    std::filesystem::create_directories(filename.parent_path(), error);
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(filename, std::ios::out | std::ios::binary);
    return file;
  }();

  const auto json = get_json_description(sprites, textures);
  if (!settings.template_file.empty()) {
    auto env = setup_inja_environment();
    env.render_to(os, env.parse_template(path_to_utf8(settings.template_file)), json);
  }
  else {
    os << json.dump(2);
  }
}

Image get_output_texture(const Settings& settings, 
    const Texture& texture, int layer_index) {
  auto target = Image(texture.width, texture.height, RGBA{ });

  auto copied_sprite = false;
  for (const auto& sprite : texture.sprites)
    copied_sprite |= copy_sprite(target, sprite, layer_index);
  if (!copied_sprite)
    return { };

  process_alpha(target, texture);

  if (settings.debug)
    for (const auto& sprite : texture.sprites)
      draw_debug_info(target, sprite);

  return target;
}

} // namespace
