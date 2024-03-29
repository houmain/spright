
#include "packing.h"

namespace spright {

void pack_origin(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Slice>& slices, bool layered) {

  for (auto& sprite : sprites) {
    sprite.trimmed_rect.x = sheet->border_padding;
    sprite.trimmed_rect.y = sheet->border_padding;
    sprite.slice_index = to_int(slices.size());
  }
  auto slice = Slice{ sheet, 0, sprites };
  slice.layered = layered;
  slices.push_back(std::move(slice));
}

} // namespace
