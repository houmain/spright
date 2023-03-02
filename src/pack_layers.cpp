
#include "packing.h"

namespace spright {

void pack_layers(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Slice>& slices) {

  for (auto& sprite : sprites) {
    sprite.trimmed_rect.x = sheet->border_padding;
    sprite.trimmed_rect.y = sheet->border_padding;
  }
  auto slice = Slice{ sheet, 0, sprites };
  slice.layered = true;
  slices.push_back(std::move(slice));
}

} // namespace
