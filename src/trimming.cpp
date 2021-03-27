
#include "trimming.h"
#include "humus/humus.h"

namespace {
  PointF normal(const PointF& v) {
    if (auto f = v.x * v.x + v.y * v.y) {
      f = 1.0f / std::sqrt(f);
      return { -v.y * f, v.x * f, };
    }
    return { 0, 0 };
  }

  void expand_convex_polygon(std::vector<PointF>& points, float distance) {
    if (distance == 0)
      return;
    distance /= 2;

    auto normals = std::vector<PointF>();
    normals.reserve(points.size());
    for (auto i = size_t{ }; i < points.size(); ++i) {
      const auto& p0 = points[i];
      const auto& p1 = points[(i + 1) % points.size()];
      normals.push_back(normal({ p1.x - p0.x, p1.y - p0.y }));
    }
    for (auto i = size_t{ }; i < points.size(); ++i) {
      const auto& n0 = normals[i];
      const auto& n1 = normals[(i - 1 + normals.size()) % normals.size()];
      points[i].x += (n0.x + n1.x) * distance;
      points[i].y += (n0.y + n1.y) * distance;
    }
  }
} // namespace

void trim_sprite(Sprite& sprite) {

  if (sprite.trim == Trim::none) {
    sprite.trimmed_source_rect = sprite.source_rect;
    return;
  }

  sprite.trimmed_source_rect = get_used_bounds(*sprite.source,
    sprite.trim_gray_levels, sprite.trim_threshold, sprite.source_rect);

  if (sprite.trim_margin)
    sprite.trimmed_source_rect = intersect(expand(
      sprite.trimmed_source_rect, sprite.trim_margin), sprite.source_rect);

  if (sprite.trim == Trim::convex) {
    const auto levels = (sprite.trim_gray_levels ?
      get_gray_levels(*sprite.source, sprite.trimmed_source_rect) :
      get_alpha_levels(*sprite.source, sprite.trimmed_source_rect));
    sprite.vertices.resize(8);
    const auto count = CreateConvexHull(sprite.trimmed_source_rect.w, sprite.trimmed_source_rect.h,
        levels.data(), sprite.trim_threshold, 16,
        static_cast<int>(sprite.vertices.size()), &sprite.vertices[0].x);
    sprite.vertices.resize(static_cast<size_t>(count));
    expand_convex_polygon(sprite.vertices, static_cast<float>(sprite.trim_margin));
  }
}

