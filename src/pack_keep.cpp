
#include "packing.h"

namespace spright {

void pack_keep(const OutputPtr& output, SpriteSpan sprites,
    std::vector<PackedTexture>& packed_textures) {

  auto max_width = 0;
  auto max_height = 0;

  for (auto& sprite : sprites) {
    sprite.trimmed_rect = sprite.trimmed_source_rect;
    sprite.rect = sprite.source_rect;
    max_width = std::max(max_width, sprite.source->width());
    max_height = std::max(max_height, sprite.source->height());
  }

  packed_textures.push_back(PackedTexture{
    output,
    0,
    max_width,
    max_height,
    sprites,
  });
}

} // namespace
