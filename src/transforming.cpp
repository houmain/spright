
#include "transforming.h"

namespace spright {

void transform_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    if (!sprite.transforms.empty()) {
      sprite.untransformed_source = sprite.source;
      sprite.untransformed_source_rect = sprite.source_rect;

      auto image = convert_to_linear(*sprite.source, sprite.source_rect);

      for (const auto& transform : sprite.transforms)
        for (const auto& step : *transform)
          std::visit(overloaded{
            [&](const TransformResize& resize) {
              image = resize_image(image, resize.size, resize.resize_filter);
            },
            [&](const TransformRotate& rotate) {
              const auto background = guess_colorkey(*sprite.source);
              image = rotate_image(image, rotate.angle, background, rotate.rotate_method);
            }
          }, step);

      sprite.source = std::make_shared<ImageFile>(
        convert_to_srgb(image),
        sprite.source->path(), sprite.source->filename());
      sprite.source_rect = sprite.source->bounds();
    }
}

void restore_untransformed_sources(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    if (sprite.untransformed_source) {
      sprite.source = std::move(sprite.untransformed_source);
      sprite.source_rect = sprite.untransformed_source_rect;
    }
}

} // namespace