
#include "packing.h"

namespace spright {

void pack_lines(bool horizontal, const OutputPtr& output,
    SpriteSpan sprites, std::vector<Texture>& textures) {

  auto texture_output_index = 0;
  const auto add_texture = [&](SpriteSpan sprites) {
    auto texture = Texture{
      output,
      texture_output_index++,
      sprites,
    };
    recompute_texture_size(texture);
    textures.push_back(std::move(texture));
  };

  const auto default_max = std::numeric_limits<int>::max();
  const auto max_width = (output->max_width ? 
    output->max_width : default_max) - output->border_padding * 2;
  const auto max_height = (output->max_height ? 
    output->max_height : default_max) - output->border_padding * 2;

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
    size = get_sprite_size(sprite);
    size.x += output->shape_padding;
    size.y += output->shape_padding;

    if (pos_d + size_d > max_d) {
      pos_d = 0;
      pos_p += line_size;
      line_size = 0;
    }
    if (pos_p + size_p > max_p) {
      add_texture({ first_sprite, it });
      first_sprite = it;
      pos.x = 0;
      pos.y = 0;
      line_size = 0;
    }
    if (pos.x + size.x > max_width ||
        pos.y + size.y > max_height)
      break;

    const auto indent = get_sprite_indent(sprite);
    sprite.trimmed_rect = {
      pos.x + indent.x + output->border_padding,
      pos.y + indent.y + output->border_padding,
      sprite.trimmed_source_rect.w,
      sprite.trimmed_source_rect.h
    };

    pos_d += size_d;
    line_size = std::max(line_size, size_p);
  }
  if (it != sprites.end())
    throw_not_all_sprites_packed();

  add_texture({ first_sprite, it });
}

} // namespace
