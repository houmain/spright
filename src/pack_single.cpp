
#include "packing.h"

void pack_single(const Texture& texture, SpriteSpan sprites,
    std::vector<PackedTexture>& packed_textures) {

  auto indices = std::map<FilenameSequence, int>();
  for (auto& sprite : sprites) {
    const auto indent = get_sprite_indent(sprite);
    const auto size = get_sprite_size(sprite);
    const auto padding = texture.border_padding;
    sprite.trimmed_rect = {
      .x = indent.x + padding,
      .y = indent.y + padding,
      .w = sprite.trimmed_source_rect.w,
      .h = sprite.trimmed_source_rect.h,
    };
    sprite.rect = {
      .x = indent.x + padding,
      .y = indent.y + padding,
      .w = size.x,
      .h = size.y,
    };
    const auto index = indices[texture.filename]++;
    if (index > texture.filename.count())
      throw std::runtime_error("not all sprites could be packed");

    packed_textures.push_back(PackedTexture{
      .filename = utf8_to_path(texture.filename.get_nth_filename(index)),
      .width = sprite.rect.w + padding * 2,
      .height = sprite.rect.h + padding * 2,
      .sprites = { &sprite, 1 },
      .alpha = texture.alpha,
      .colorkey = texture.colorkey,
    });
  }
}
