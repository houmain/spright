
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

} // namespace

void output_definition(const Settings& settings, const std::vector<Sprite>& sprites) {
  if (settings.output_file.empty() || sprites.empty())
    return;

  auto json = nlohmann::json{ };
  auto& json_sprites = json["sprites"];

  using TagKey = std::pair<std::string, std::string>;
  using SpriteIndex = size_t;
  auto tags = std::map<TagKey, std::vector<SpriteIndex>>();

  for (const auto& sprite : sprites) {
    auto& json_sprite = json_sprites.emplace_back();
    json_sprite["id"] = sprite.id;
    json_sprite["rect"] = json_rect(sprite.rect);
    json_sprite["trimmedRect"] = json_rect(sprite.trimmed_rect);
    json_sprite["source"] = sprite.source->filename();
    json_sprite["sourceRect"] = json_rect(sprite.source_rect);
    json_sprite["trimmedSourceRect"] = json_rect(sprite.trimmed_source_rect);
    json_sprite["pivot"] = json_point(sprite.pivot_point);
    json_sprite["trimmedPivot"] = json_point(sprite.trimmed_pivot_point);
    json_sprite["margin"] = sprite.margin;
    json_sprite["tags"] = sprite.tags;
    for (const auto& tag_key : sprite.tags)
      tags[tag_key].push_back(json_sprites.size() - 1);
  }

  auto& json_tags = json["tags"];
  for (const auto& [tag_key, sprite_indices] : tags) {
    auto& json_tag = json_tags.emplace_back();
    json_tag["id"] = tag_key.first;
    if (!tag_key.second.empty())
      json_tag["value"] = tag_key.second;
    auto& json_tag_sprites = json_tag["sprites"];
    for (auto index : sprite_indices)
      json_tag_sprites.push_back(json_sprites[index]);
  }

  auto file = std::ofstream();
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(settings.output_file, std::ios::out);

  if (!settings.template_file.empty()) {
    auto env = inja::Environment();
    env.set_trim_blocks(true);
    env.set_lstrip_blocks(true);
    env.render_to(file, env.parse_template(settings.template_file.u8string()), json);
  }
  else {
    file << json.dump(2);
  }
}