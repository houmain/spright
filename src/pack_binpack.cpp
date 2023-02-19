
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
    auto size = get_sprite_size(sprite);
    size.x += sheet.shape_padding;
    size.y += sheet.shape_padding;
    pack_sizes.push_back({ to_int(pack_sizes.size()), size.x, size.y });
  }

  const auto [max_slice_width, max_slice_height] = get_slice_max_size(sheet);
  const auto pack_sheets = pack(
    rect_pack::Settings{
      (fast ? rect_pack::Method::Best_Skyline : rect_pack::Method::Best),
      get_max_slice_count(sheet),
      sheet.power_of_two,
      sheet.square,
      sheet.allow_rotate,
      sheet.align_width,
      sheet.border_padding,
      sheet.shape_padding,
      sheet.width,
      sheet.height,
      max_slice_width,
      max_slice_height,
    },
    std::move(pack_sizes));

  // update sprite rects
  auto slice_index = 0;
  auto packed_sprites = size_t{ };
  for (const auto& pack_sheet : pack_sheets) {
    for (const auto& pack_rect : pack_sheet.rects) {
      auto& sprite = sprites[to_unsigned(pack_rect.id)];
      const auto indent = get_sprite_indent(sprite);
      sprite.rotated = pack_rect.rotated;
      sprite.slice_index = slice_index;
      sprite.trimmed_rect = {
        pack_rect.x + indent.x,
        pack_rect.y + indent.y,
        sprite.trimmed_source_rect.w,
        sprite.trimmed_source_rect.h
      };
      ++packed_sprites;
    }
    ++slice_index;
  }

  if (packed_sprites < sprites.size())
    throw_not_all_sprites_packed();

  create_slices_from_indices(sheet_ptr, sprites, slices);

  auto slice = &slices[slices.size() - pack_sheets.size()];
  for (const auto& pack_sheet : pack_sheets) {
    slice->width = pack_sheet.width;
    slice->height = pack_sheet.height;
    ++slice;
  }
}

} // namespace
