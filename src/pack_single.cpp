
#include "packing.h"

namespace spright {

void pack_single(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Texture>& textures) {

  auto sheet_index = 0;
  for (auto& sprite : sprites) {
    const auto indent = get_sprite_indent(sprite);
    const auto size = get_sprite_size(sprite);
    const auto padding = sheet->border_padding;
    sprite.trimmed_rect = {
      indent.x + padding,
      indent.y + padding,
      sprite.trimmed_source_rect.w,
      sprite.trimmed_source_rect.h,
    };
    sprite.rect = {
      indent.x + padding,
      indent.y + padding,
      size.x,
      size.y,
    };
    textures.push_back(Texture{
      sheet,
      sheet_index++,
      { &sprite, 1 },
      sprite.rect.w + padding * 2,
      sprite.rect.h + padding * 2,
    });
  }
}

} // namespace
