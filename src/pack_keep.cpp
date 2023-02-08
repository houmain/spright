
#include "packing.h"

namespace spright {

void pack_keep(const OutputPtr& output, SpriteSpan sprites,
    std::vector<Texture>& textures) {

  auto source_indices = std::map<ImagePtr, size_t>();
  for (auto& sprite : sprites) {
    const auto source_index = source_indices.emplace(
      sprite.source, source_indices.size()).first->second;
    sprite.texture_filename_index = source_index;
    sprite.trimmed_rect = sprite.trimmed_source_rect;
    sprite.rect = sprite.source_rect;
  }

  create_textures_from_filename_indices(
    output, sprites, textures);

  for (auto& texture : textures)
    recompute_texture_size(texture);
}

} // namespace
