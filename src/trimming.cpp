
#include "trimming.h"
#include "humus/humus.h"

void trim_sprite(Sprite& sprite) {

  if (sprite.trim == Trim::none) {
    sprite.trimmed_source_rect = sprite.source_rect;
    return;
  }

  sprite.trimmed_source_rect =
    get_used_bounds(*sprite.source, sprite.source_rect, sprite.trim_threshold);

  if (sprite.trim == Trim::convex) {
    const auto alpha = get_alpha_levels(*sprite.source, sprite.trimmed_source_rect);
    sprite.vertices.resize(8);
    const auto count = CreateConvexHull(sprite.trimmed_source_rect.w, sprite.trimmed_source_rect.h,
        alpha.data(), sprite.trim_threshold, 8,
        static_cast<int>(sprite.vertices.size()), &sprite.vertices[0].x);
    sprite.vertices.resize(static_cast<size_t>(count));
  }

  if (sprite.trim_margin)
    sprite.trimmed_source_rect = intersect(expand(
      sprite.trimmed_source_rect, sprite.trim_margin), sprite.source_rect);
}

