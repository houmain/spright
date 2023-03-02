
#include "packing.h"

namespace spright {

void pack_lines(bool horizontal, const SheetPtr& sheet,
    SpriteSpan sprites, std::vector<Slice>& slices) {

  auto slice_sheet_index = 0;
  const auto add_slice = [&](SpriteSpan sprites) {
    slices.push_back({
      sheet,
      slice_sheet_index++,
      sprites,
    });
  };

  auto [max_width, max_height] = get_slice_max_size(*sheet);
  max_width -= sheet->border_padding * 2;
  max_height -= sheet->border_padding * 2;
  auto pos = Point{ };
  auto size = Size{ };
  auto line_size = 0;

  // d = direction, p = perpendicular
  auto& pos_d = (horizontal ? pos.x : pos.y);
  auto& pos_p = (horizontal ? pos.y : pos.x);
  const auto& size_d = (horizontal ? size.x : size.y);
  const auto& size_p = (horizontal ? size.y : size.x);
  const auto max_d = (horizontal ? max_width : max_height);
  const auto max_p = (horizontal ? max_height : max_width);

  auto first_sprite = sprites.begin();
  auto it = first_sprite;
  for (; it != sprites.end(); ++it) {
    auto& sprite = *it;
    size = sprite.bounds;

    if (pos_d + size_d > max_d) {
      pos_d = 0;
      pos_p += line_size;
      line_size = 0;
    }
    if (pos_p + size_p > max_p) {
      add_slice({ first_sprite, it });
      first_sprite = it;
      pos.x = 0;
      pos.y = 0;
      line_size = 0;
    }
    if (pos.x + size.x > max_width ||
        pos.y + size.y > max_height)
      break;

    sprite.trimmed_rect.x = pos.x + sheet->border_padding;
    sprite.trimmed_rect.y = pos.y + sheet->border_padding;

    pos_d += size_d + sheet->shape_padding;
    line_size = std::max(line_size, size_p + sheet->shape_padding);
  }
  if (it != sprites.end())
    throw_not_all_sprites_packed();

  add_slice({ first_sprite, it });
}

} // namespace
