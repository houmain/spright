
#include "packing.h"
#include "rect_pack/rect_pack.h"

namespace spright {

void pack_binpack(const OutputPtr& output_ptr, SpriteSpan sprites,
    bool fast, std::vector<Texture>& textures) {
  const auto& output = *output_ptr;

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
  auto texture_filename_index = 0;
  auto packed_sprites = size_t{ };
  for (const auto& pack_sheet : pack_sheets) {
    for (const auto& pack_rect : pack_sheet.rects) {
      auto& sprite = sprites[static_cast<size_t>(pack_rect.id)];
      const auto indent = get_sprite_indent(sprite);
      sprite.rotated = pack_rect.rotated;;
      sprite.texture_filename_index = texture_filename_index;
      sprite.trimmed_rect = {
        pack_rect.x + indent.x,
        pack_rect.y + indent.y,
        sprite.trimmed_source_rect.w,
        sprite.trimmed_source_rect.h
      };
      ++packed_sprites;
    }
    ++texture_filename_index;
  }

  if (packed_sprites < sprites.size())
    throw_not_all_sprites_packed();

  create_textures_from_filename_indices(output_ptr, sprites, textures);

  for (auto i = size_t{ }; i < textures.size(); i++) {
    auto& texture = textures[i];
    const auto& pack_sheet = pack_sheets[texture.filename_index];
    texture.width = pack_sheet.width;
    texture.height = pack_sheet.height;
  }
}

} // namespace
