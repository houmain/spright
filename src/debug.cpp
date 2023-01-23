
#include "debug.h"

namespace spright {

namespace {
  void draw_sprite_info(Image& target, const Sprite& sprite) {
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

void draw_debug_info(Image& image, const Texture &texture) {
  for (const auto& sprite : texture.sprites)
    draw_sprite_info(image, sprite);
}

} // namespace
