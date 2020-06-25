
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
  auto json_sprites = nlohmann::json::array();
  auto tags = std::map<std::string, std::vector<std::string>>();

  for (const auto& sprite : sprites) {
    auto json_sprite = nlohmann::json::object();
    const auto id = (!sprite.id.empty() ? sprite.id :
      "sprite_" + std::to_string(json_sprites.size()));
    json_sprite["id"] = id;
    json_sprite["rect"] = json_rect(sprite.rect);
    json_sprite["trimmedRect"] = json_rect(sprite.trimmed_rect);
    json_sprite["source"] = sprite.source->filename();
    json_sprite["sourceRect"] = json_rect(sprite.source_rect);
    json_sprite["trimmedSourceRect"] = json_rect(sprite.trimmed_source_rect);
    json_sprite["pivot"] = json_point(sprite.pivot_point);
    json_sprite["trimmedPivot"] = json_point(sprite.trimmed_pivot_point);
    json_sprite["margin"] = sprite.margin;
    json_sprite["tags"] = sprite.tags;
    for (const auto& tag : sprite.tags)
      tags[tag].push_back(id);
    json_sprites.push_back(std::move(json_sprite));
  }
  json["sprites"] = std::move(json_sprites);

  auto json_tags = nlohmann::json::array();
  for (const auto& [id, sprite_ids] : tags) {
    auto json_tag = nlohmann::json::object();
    json_tag["id"] = id;
    json_tag["spriteIds"] = sprite_ids;
    json_tags.push_back(std::move(json_tag));
  }
  json["tags"] = std::move(json_tags);

  auto file = std::ofstream();
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  file.open(settings.output_file, std::ios::out);

  if (!settings.template_file.empty()) {
    auto env = inja::Environment();
    env.render_to(file, env.parse_template(settings.template_file), json);
  }
  else {
    file << json.dump(2);
  }
}
