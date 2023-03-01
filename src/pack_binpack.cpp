
#include "packing.h"
#include "rect_pack/rect_pack.h"

namespace spright {

void pack_binpack(const SheetPtr& sheet_ptr, SpriteSpan sprites,
    bool fast, std::vector<Slice>& slices) {
  const auto& sheet = *sheet_ptr;

  // pack rects
  auto pack_sizes = std::vector<rect_pack::Size>();
  pack_sizes.reserve(sprites.size());
  for (const auto& sprite : sprites) {
    auto size = sprite.size;
    size.x += sheet.shape_padding;
    size.y += sheet.shape_padding;
    pack_sizes.push_back({ to_int(pack_sizes.size()), size.x, size.y });
  }

  const auto [max_width, max_height] = get_slice_max_size(sheet);
  const auto pack_sheets = pack(
    rect_pack::Settings{
      (fast ? rect_pack::Method::Best_Skyline : rect_pack::Method::Best),
      get_max_slice_count(sheet),
      sheet.power_of_two,
      sheet.square,
      sheet.allow_rotate,
      sheet.divisible_width,
      sheet.border_padding,
      sheet.shape_padding,
      sheet.width,
      sheet.height,
      max_width,
      max_height,
    },
    std::move(pack_sizes));

  // update sprite rects
  auto slice_index = 0;
  auto packed_sprites = size_t{ };
  for (const auto& pack_sheet : pack_sheets) {
    for (const auto& pack_rect : pack_sheet.rects) {
      auto& sprite = sprites[to_unsigned(pack_rect.id)];
      sprite.rotated = pack_rect.rotated;
      sprite.slice_index = slice_index;
      sprite.trimmed_rect.x = pack_rect.x + sprite.offset.x;
      sprite.trimmed_rect.y = pack_rect.y + sprite.offset.y;
      ++packed_sprites;
    }
    ++slice_index;
  }

  if (packed_sprites < sprites.size())
    throw_not_all_sprites_packed();

  create_slices_from_indices(sheet_ptr, sprites, slices);
}

} // namespace
