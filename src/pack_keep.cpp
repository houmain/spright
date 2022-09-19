
#include "packing.h"

namespace spright {

void pack_keep(const Output& output, SpriteSpan sprites,
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
    utf8_to_path(output.filename.get_nth_filename(0)),
    max_width,
    max_height,
    sprites,
    output.alpha,
    output.colorkey,
  });
}

} // namespace
