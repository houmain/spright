
#include "packing.h"

namespace spright {

void pack_single(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Slice>& slices) {

  auto sheet_index = 0;
  for (auto& sprite : sprites) {
    sprite.trimmed_rect.x = sheet->border_padding;
    sprite.trimmed_rect.y = sheet->border_padding;
    sprite.slice_index = to_int(slices.size());
    slices.push_back({
      sheet,
      sheet_index++,
      { &sprite, 1 },
    });
  }
}

} // namespace
