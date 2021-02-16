
#include "output.h"
#include "inja/inja.hpp"
#include <fstream>

namespace {
  nlohmann::json json_point(const PointF& point) {
    auto json_point = nlohmann::json::object();
    json_point["x"] = point.x;
    json_point["y"] = point.y;
    return json_point;
  }

  nlohmann::json json_rect(const Rect& rect) {
    auto json_rect = nlohmann::json::object();
    json_rect["x"] = rect.x;
    json_rect["y"] = rect.y;
    json_rect["w"] = rect.w;
    json_rect["h"] = rect.h;
    return json_rect;
  };

  nlohmann::json get_json_description(const std::vector<Sprite>& sprites, const std::vector<PackedTexture>& textures) {
    auto json = nlohmann::json{ };
    auto& json_sprites = json["sprites"];
    json_sprites = nlohmann::json::array();

    using TagKey = std::pair<std::string, std::string>;
    using SpriteIndex = size_t;
    auto tags = std::map<TagKey, std::vector<SpriteIndex>>();
    auto texture_sprites = std::map<std::filesystem::path, std::vector<SpriteIndex>>();

    for (const auto& sprite : sprites) {
      auto& json_sprite = json_sprites.emplace_back();
      const auto index = json_sprites.size() - 1;
      const auto texture_filename = utf8_to_path(
        sprite.texture->filename.get_nth_filename(sprite.texture_index));
      json_sprite["id"] = sprite.id;
      json_sprite["rect"] = json_rect(sprite.rect);
      json_sprite["trimmedRect"] = json_rect(sprite.trimmed_rect);
      json_sprite["sourceFilename"] = path_to_utf8(sprite.source->filename());
      json_sprite["sourcePath"] = path_to_utf8(sprite.source->path());
      json_sprite["sourceRect"] = json_rect(sprite.source_rect);
      json_sprite["trimmedSourceRect"] = json_rect(sprite.trimmed_source_rect);
      json_sprite["pivot"] = json_point(sprite.pivot_point);
      json_sprite["trimmedPivot"] = json_point(sprite.trimmed_pivot_point);
      json_sprite["textureFilename"] = path_to_utf8(texture_filename);
      json_sprite["rotated"] = sprite.rotated;
      json_sprite["tags"] = sprite.tags;
      for (const auto& tag_key : sprite.tags)
        tags[tag_key].push_back(index);
      texture_sprites[texture_filename].push_back(index);
    }

    auto& json_tags = json["tags"];
    json_tags = nlohmann::json::array();
    for (const auto& [tag_key, sprite_indices] : tags) {
      auto& json_tag = json_tags.emplace_back();
      json_tag["id"] = tag_key.first;
      if (!tag_key.second.empty())
        json_tag["value"] = tag_key.second;
      auto& json_tag_sprites = json_tag["sprites"];
      for (auto index : sprite_indices)
        json_tag_sprites.push_back(json_sprites[index]);
    }

    auto& json_textures = json["textures"];
    json_textures = nlohmann::json::array();
    for (const auto& texture : textures) {
      auto& json_texture = json_textures.emplace_back();
      json_texture["filename"] = path_to_utf8(texture.filename);
      json_texture["width"] = texture.width;
      json_texture["height"] = texture.height;
      auto& json_texture_sprites = json_texture["sprites"];
      for (auto index : texture_sprites[texture.filename])
        json_texture_sprites.push_back(json_sprites[index]);
    }
    return json;
  }
} // namespace

void output_description(const Settings& settings,
    const std::vector<Sprite>& sprites, const std::vector<PackedTexture>& textures) {
  if (settings.output_file.empty())
    return;

  auto file = std::ofstream();
  auto& os = [&]() -> std::ostream& {
    if (settings.output_file == "cout")
      return std::cout;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(settings.output_file, std::ios::out);
    return file;
  }();

  const auto json = get_json_description(sprites, textures);
  if (!settings.template_file.empty()) {
    auto env = inja::Environment();
    env.set_trim_blocks(false);
    env.set_lstrip_blocks(false);
    env.render_to(os, env.parse_template(path_to_utf8(settings.template_file)), json);
  }
  else {
    os << json.dump(2);
  }
}

void output_texture(const Settings& settings, const PackedTexture& texture) {
  // copy from sources to target sheet
  auto target = Image(texture.width, texture.height);
  for (const auto& sprite : texture.sprites) {
    if (sprite.rotated) {
      copy_rect_rotated_cw(*sprite.source, sprite.trimmed_source_rect,
        target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
    }
    else {
      copy_rect(*sprite.source, sprite.trimmed_source_rect,
        target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
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
  }

  switch (texture.alpha) {
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
      make_opaque(target, texture.colorkey);
      break;
  }

  // draw debug info
  if (settings.debug) {
    for (const auto& sprite : texture.sprites) {
      auto rect = sprite.rect;
      auto trimmed_rect = sprite.trimmed_rect;
      auto pivot_point = sprite.pivot_point;
      if (sprite.rotated) {
        std::swap(rect.w, rect.h);
        std::swap(trimmed_rect.w, trimmed_rect.h);
        std::swap(pivot_point.x, pivot_point.y);
        pivot_point.x = (static_cast<float>(rect.w-1) - pivot_point.x);
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
    }
  }

  auto error = std::error_code{ };
  std::filesystem::create_directories(texture.filename.parent_path(), error);
  save_image(target, texture.filename);
}
