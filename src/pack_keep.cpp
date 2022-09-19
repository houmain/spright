
#include "packing.h"

namespace spright {

void pack_keep(const OutputPtr& output, SpriteSpan sprites,
    std::vector<Texture>& textures) {

  auto max_width = 0;
  auto max_height = 0;

  for (auto& sprite : sprites) {
    sprite.trimmed_rect = sprite.trimmed_source_rect;
    sprite.rect = sprite.source_rect;
    max_width = std::max(max_width, sprite.source->width());
    max_height = std::max(max_height, sprite.source->height());
  }

  textures.push_back(Texture{
    output,
    0,
    max_width,
    max_height,
    sprites,
  });
}

} // namespace
