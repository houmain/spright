
#include "packing.h"

void pack_keep(const Texture& texture, SpriteSpan sprites,
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
    .filename = utf8_to_path(texture.filename.get_nth_filename(0)),
    .width = max_width,
    .height = max_height,
    .sprites = sprites,
    .alpha = texture.alpha,
    .colorkey = texture.colorkey,
  });
}
