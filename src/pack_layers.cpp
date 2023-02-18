
#include "packing.h"

namespace spright {

void pack_layers(const OutputPtr& output, SpriteSpan sprites,
    std::vector<Texture>& textures) {

  for (auto& sprite : sprites) {
    const auto indent = get_sprite_indent(sprite);
    const auto size = get_sprite_size(sprite);
    const auto padding = output->border_padding;
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
  }
  auto texture = Texture{ output, 0, sprites };
  texture.layered = true;
  recompute_texture_size(texture);  
  textures.push_back(std::move(texture));
}

} // namespace
