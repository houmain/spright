
#include "packing.h"
#include "rect_pack/rect_pack.h"

namespace spright {

void pack_binpack(const Output& output, std::span<Sprite> sprites,
    bool fast, std::vector<PackedTexture>& packed_textures) {
  // pack rects
  auto pack_sizes = std::vector<rect_pack::Size>();
  pack_sizes.reserve(sprites.size());
  for (const auto& sprite : sprites) {
    auto size = get_sprite_size(sprite);
    size.x += output.shape_padding;
    size.y += output.shape_padding;
    pack_sizes.push_back({ static_cast<int>(pack_sizes.size()), size.x, size.y });
  }

  const auto [max_texture_width, max_texture_height] = get_texture_max_size(output);
  const auto pack_sheets = pack(
    rect_pack::Settings{
      (fast ? rect_pack::Method::Best_Skyline : rect_pack::Method::Best),
      output.filename.count(),
      output.power_of_two,
      output.square,
      output.allow_rotate,
      output.align_width,
      output.border_padding,
      output.shape_padding,
      output.width,
      output.height,
      max_texture_width,
      max_texture_height,
    },
    std::move(pack_sizes));

  // update sprite rects
  auto texture_index = 0;
  auto packed_sprites = size_t{ };
  for (const auto& pack_sheet : pack_sheets) {
    for (const auto& pack_rect : pack_sheet.rects) {
      auto& sprite = sprites[static_cast<size_t>(pack_rect.id)];
      const auto indent = get_sprite_indent(sprite);
      sprite.rotated = pack_rect.rotated;;
      sprite.texture_index = texture_index;
      sprite.trimmed_rect = {
        pack_rect.x + indent.x,
        pack_rect.y + indent.y,
        sprite.trimmed_source_rect.w,
        sprite.trimmed_source_rect.h
      };
      ++packed_sprites;
    }
    ++texture_index;
  }

  if (packed_sprites < sprites.size())
    throw std::runtime_error("not all sprites could be packed");

  // sort sprites by output index
  if (pack_sheets.size() > 1)
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.texture_index, a.index) <
               std::tie(b.texture_index, b.index);
      });

  // add to output textures
  auto texture_begin = sprites.begin();
  const auto end = sprites.end();
  for (auto it = texture_begin;; ++it)
    if (it == end || it->texture_index != texture_begin->texture_index) {
      const auto sheet_index = texture_begin->texture_index;
      const auto sheet_sprites = SpriteSpan(texture_begin, it);
      const auto& pack_sheet = pack_sheets[static_cast<size_t>(sheet_index)];
      packed_textures.push_back(PackedTexture{
        output.filename.get_nth_filename(sheet_index),
        pack_sheet.width,
        pack_sheet.height,
        sheet_sprites,
        output.alpha,
        output.colorkey,
      });

      texture_begin = it;
      if (it == end)
        break;
    }
}

} // namespace
