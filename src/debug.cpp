
#include "debug.h"

namespace spright {

namespace {
  Rect round(const RectF& rect) {
    return Rect{ 
      static_cast<int>(rect.x + 0.5f), 
      static_cast<int>(rect.y + 0.5f), 
      std::max(static_cast<int>(rect.w + 0.5f), 1), 
      std::max(static_cast<int>(rect.h + 0.5f), 1)
    };
  }

  Point round(const PointF& point) {
    return Point{
      static_cast<int>(point.x + 0.5f), 
      static_cast<int>(point.y + 0.5f), 
    };
  }

  void draw_point(Image& target, const PointF& point, const RGBA& color) {
    // floor coordinate
    auto [l, t] = Point(point);
    auto [r, b] = Point(point);

    // extend to even size when it does not round towards 0.5
    if (static_cast<int>(point.x * 2) % 2 == 0)
      l -= 1;
    if (static_cast<int>(point.y * 2) % 2 == 0)
      t -= 1;

    draw_line(target, Point(l, t) + Point(-1, 0), 
                      Point(l, t) + Point( 0,-1), color);
    draw_line(target, Point(r, t) + Point( 1, 0), 
                      Point(r, t) + Point( 0,-1), color);
    draw_line(target, Point(l, b) + Point(-1, 0), 
                      Point(l, b) + Point( 0, 1), color);
    draw_line(target, Point(r, b) + Point( 1, 0), 
                      Point(r, b) + Point( 0, 1), color);
  }

  void draw_sprite_info(Image& target, const Sprite& sprite, float scale) {
    const auto scale_rect = [&](const auto& rect) {
      return RectF{ 
        static_cast<float>(rect.x) * scale,
        static_cast<float>(rect.y) * scale,
        static_cast<float>(rect.w) * scale,
        static_cast<float>(rect.h) * scale
      };
    };
    const auto scale_point = [&](const auto& point) {
      return PointF{
        static_cast<float>(point.x) * scale,
        static_cast<float>(point.y) * scale
      };
    };

    auto rect = scale_rect(sprite.rect);
    auto trimmed_rect = scale_rect(sprite.trimmed_rect);
    auto pivot_point = scale_point(sprite.pivot_point);
    if (sprite.rotated) {
      std::swap(rect.w, rect.h);
      std::swap(trimmed_rect.w, trimmed_rect.h);
      pivot_point = rotate_cw(pivot_point, rect.w - 1.0f);
    }
    draw_rect(target, round(rect), RGBA{ { 255, 0, 255, 128 } });
    draw_rect(target, round(trimmed_rect), RGBA{ { 255, 255, 0, 128 } });
    
    // scale point coordinates, so bottom right is inside sprite
    // (similar to 1.0 for texture coordinates...)
    const auto scale_point_coord = PointF(
      scale - scale / trimmed_rect.w, 
      scale - scale / trimmed_rect.h);

    if (!sprite.vertices.empty()) {
      const auto origin = scale_point(sprite.trimmed_rect.xy());
      for (auto i = 0u; i < sprite.vertices.size(); i++) {
        auto v0 = sprite.vertices[i];
        auto v1 = sprite.vertices[(i + 1) % sprite.vertices.size()];
        if (sprite.rotated) {
          v0 = rotate_cw(v0, sprite.trimmed_rect.w);
          v1 = rotate_cw(v1, sprite.trimmed_rect.w);
        }
        v0.x *= scale_point_coord.x;
        v0.y *= scale_point_coord.y;
        v1.x *= scale_point_coord.x;
        v1.y *= scale_point_coord.y;
        draw_line_stipple(target,
          round(origin + v0),
          round(origin + v1),
          RGBA{ { 0, 255, 255, 196 } },
          2,
          true);
      }
    }

    draw_point(target, rect.xy() + pivot_point, RGBA{ { 255, 0, 0, 255 } });
  }
} // namespace

void draw_debug_info(Image& image, const Texture &texture, float scale) {
  for (const auto& sprite : texture.sprites)
    draw_sprite_info(image, sprite, scale);
}

} // namespace
