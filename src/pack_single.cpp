
#include "packing.h"

namespace spright {

void pack_single(const Texture& texture, SpriteSpan sprites,
    std::vector<PackedTexture>& packed_textures) {

  auto indices = std::map<FilenameSequence, int>();
  for (auto& sprite : sprites) {
    const auto indent = get_sprite_indent(sprite);
    const auto size = get_sprite_size(sprite);
    const auto padding = texture.border_padding;
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
    const auto index = indices[texture.filename]++;
    if (index > texture.filename.count())
      throw std::runtime_error("not all sprites could be packed");

    packed_textures.push_back(PackedTexture{
      utf8_to_path(texture.filename.get_nth_filename(index)),
      sprite.rect.w + padding * 2,
      sprite.rect.h + padding * 2,
      { &sprite, 1 },
      texture.alpha,
      texture.colorkey,
    });
  }
}

} // namespace
