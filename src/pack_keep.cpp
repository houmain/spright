
#include "packing.h"

namespace spright {

void pack_keep(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Slice>& slices) {

  auto source_indices = std::map<ImageFilePtr, int>();
  for (auto& sprite : sprites) {
    const auto source_index = source_indices.emplace(
      sprite.source, to_int(source_indices.size())).first->second;
    sprite.slice_index = source_index;
    sprite.trimmed_rect = sprite.trimmed_source_rect;
  }

  create_slices_from_indices(sheet, sprites, slices);
}

} // namespace
