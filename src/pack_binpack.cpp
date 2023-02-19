
#include "packing.h"
#include "rect_pack/rect_pack.h"

namespace spright {

void pack_binpack(const SheetPtr& sheet_ptr, SpriteSpan sprites,
    bool fast, std::vector<Texture>& textures) {
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

  const auto [max_texture_width, max_texture_height] = get_texture_max_size(sheet);
  const auto pack_sheets = pack(
    rect_pack::Settings{
      (fast ? rect_pack::Method::Best_Skyline : rect_pack::Method::Best),
      get_max_texture_count(sheet),
      sheet.power_of_two,
      sheet.square,
      sheet.allow_rotate,
      sheet.align_width,
      sheet.border_padding,
      sheet.shape_padding,
      sheet.width,
      sheet.height,
      max_texture_width,
      max_texture_height,
    },
    std::move(pack_sizes));

  // update sprite rects
  auto texture_sheet_index = 0;
  auto packed_sprites = size_t{ };
  for (const auto& pack_sheet : pack_sheets) {
    for (const auto& pack_rect : pack_sheet.rects) {
      auto& sprite = sprites[to_unsigned(pack_rect.id)];
      const auto indent = get_sprite_indent(sprite);
      sprite.rotated = pack_rect.rotated;
      sprite.texture_sheet_index = texture_sheet_index;
      sprite.trimmed_rect = {
        pack_rect.x + indent.x,
        pack_rect.y + indent.y,
        sprite.trimmed_source_rect.w,
        sprite.trimmed_source_rect.h
      };
      ++packed_sprites;
    }
    ++texture_sheet_index;
  }

  if (packed_sprites < sprites.size())
    throw_not_all_sprites_packed();

  create_textures_from_filename_indices(sheet_ptr, sprites, textures);

  auto texture = &textures[textures.size() - pack_sheets.size()];
  for (const auto& pack_sheet : pack_sheets) {
    texture->width = pack_sheet.width;
    texture->height = pack_sheet.height;
    ++texture;
  }
}

} // namespace
