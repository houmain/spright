
#include "packing.h"

namespace spright {

namespace {
  int get_max_size(int size, int max_size, bool power_of_two) {
    if (power_of_two && size)
      size = ceil_to_pot(size);

    if (power_of_two && max_size)
      max_size = floor_to_pot(max_size);

    if (size > 0 && max_size > 0)
      return std::min(size, max_size);
    if (size > 0)
      return size;
    if (max_size > 0)
      return max_size;
    return std::numeric_limits<int>::max();
  }

  void prepare_sprite(Sprite& sprite) {
    const auto distance_to_next_multiple =
      [](int value, int divisor) { return ceil(value, divisor) - value; };
    sprite.common_divisor_margin = {
      distance_to_next_multiple(sprite.trimmed_source_rect.w, sprite.common_divisor.x),
      distance_to_next_multiple(sprite.trimmed_source_rect.h, sprite.common_divisor.y),
    };
    sprite.common_divisor_offset = {
      sprite.common_divisor_margin.x / 2,
      sprite.common_divisor_margin.y / 2
    };
  }

  void complete_sprite(Sprite& sprite) {
    auto& rect = sprite.rect;
    if (sprite.crop) {
      rect = sprite.trimmed_rect;
    }
    else {
      rect = {
        sprite.trimmed_rect.x - (sprite.trimmed_source_rect.x - sprite.source_rect.x),
        sprite.trimmed_rect.y - (sprite.trimmed_source_rect.y - sprite.source_rect.y),
        sprite.source_rect.w,
        sprite.source_rect.h,
      };
    }
    rect.x -= sprite.common_divisor_offset.x;
    rect.y -= sprite.common_divisor_offset.y;
    rect.w += sprite.common_divisor_margin.x;
    rect.h += sprite.common_divisor_margin.y;

    auto& pivot_point = sprite.pivot_point;
    const auto pivot_rect = RectF(sprite.crop_pivot ?
      sprite.trimmed_source_rect : sprite.source_rect);
    switch (sprite.pivot.x) {
      case PivotX::left: pivot_point.x += 0; break;
      case PivotX::center: pivot_point.x += pivot_rect.w / 2; break;
      case PivotX::right: pivot_point.x += pivot_rect.w; break;
    }
    switch (sprite.pivot.y) {
      case PivotY::top: pivot_point.y += 0; break;
      case PivotY::middle: pivot_point.y += pivot_rect.h / 2; break;
      case PivotY::bottom: pivot_point.y += pivot_rect.h; break;
    }
    pivot_point.x -= rect.x - sprite.trimmed_rect.x;
    pivot_point.y -= rect.y - sprite.trimmed_rect.y;
    pivot_point.x += (pivot_rect.x - sprite.trimmed_source_rect.x);
    pivot_point.y += (pivot_rect.y - sprite.trimmed_source_rect.y);
  }

  void pack_texture(const OutputPtr& output,
      SpriteSpan sprites, std::vector<Texture>& textures) {
    assert(!sprites.empty());

    switch (output->pack) {
      case Pack::binpack: return pack_binpack(output, sprites, sprites.size() > 1000, textures);
      case Pack::compact: return pack_compact(output, sprites, textures);
      case Pack::single: return pack_single(output, sprites, textures);
      case Pack::keep: return pack_keep(output, sprites, textures);
      case Pack::rows: return pack_lines(true, output, sprites, textures);
      case Pack::columns: return pack_lines(false, output, sprites, textures);
    }
  }

  void pack_texture_deduplicate(const OutputPtr& output,
      SpriteSpan sprites, std::vector<Texture>& textures) {
    assert(!sprites.empty());

    // sort duplicates to back
    auto unique_sprites = sprites;
    for (auto i = size_t{ }; i < unique_sprites.size(); ++i) {
      for (auto j = size_t{ }; j < i; ++j)
        if (is_identical(*sprites[i].source, sprites[i].trimmed_source_rect,
                         *sprites[j].source, sprites[j].trimmed_source_rect)) {
          sprites[i].duplicate_of_index = sprites[j].index;
          std::swap(sprites[i--], unique_sprites.back());
          unique_sprites = unique_sprites.first(unique_sprites.size() - 1);
          break;
        }
       }

    // restore order of unique sprites before packing
    std::sort(unique_sprites.begin(), unique_sprites.end(),
      [](const Sprite& a, const Sprite& b) { return (a.index < b.index); });

    pack_texture(output, unique_sprites, textures);

    const auto duplicate_sprites = sprites.last(sprites.size() - unique_sprites.size());
    if (output->duplicates == Duplicates::drop) {
      for (auto& sprite : duplicate_sprites)
        sprite = { };
    }
    else {
      // copy rectangles from unique to duplicate sprites
      auto sprites_by_id = std::map<int, const Sprite*>();
      for (const auto& sprite : unique_sprites)
        sprites_by_id[sprite.index] = &sprite;
      for (auto& duplicate : duplicate_sprites) {
        const auto& sprite = *sprites_by_id.find(duplicate.duplicate_of_index)->second;
        duplicate.texture_filename_index = sprite.texture_filename_index;
        duplicate.trimmed_rect = sprite.trimmed_rect;
        duplicate.rotated = sprite.rotated;
      }

      // restore order of duplicate sprites
      std::sort(std::begin(duplicate_sprites), std::end(duplicate_sprites),
        [](const Sprite& a, const Sprite& b) { return a.index < b.index; });
    }
  }

  std::vector<Texture> pack_sprites_by_output(SpriteSpan sprites) {
    if (sprites.empty())
      return { };

    // sort sprites by output
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.output->filename, a.index) <
               std::tie(b.output->filename, b.index);
      });

    auto textures = std::vector<Texture>();
    for (auto begin = sprites.begin(), it = begin; ; ++it)
      if (it == sprites.end() ||
          it->output->filename != begin->output->filename) {
        const auto& output = begin->output;
        if (output->duplicates != Duplicates::keep)
          pack_texture_deduplicate(output, { begin, it }, textures);
        else
          pack_texture(output, { begin, it }, textures);

        if (it == sprites.end())
          break;
        begin = it;
      }
    return textures;
  }
} // namespace

std::pair<int, int> get_texture_max_size(const Output& output) {
  return {
    get_max_size(output.width, output.max_width, output.power_of_two),
    get_max_size(output.height, output.max_height, output.power_of_two)
  };
}

Size get_sprite_size(const Sprite& sprite) {
  return {
    sprite.trimmed_source_rect.w + sprite.common_divisor_margin.x + 
      sprite.extrude.count * 2,
    sprite.trimmed_source_rect.h + sprite.common_divisor_margin.y + 
      sprite.extrude.count * 2
  };
}

Size get_sprite_indent(const Sprite& sprite) {
  return {
    sprite.common_divisor_offset.x + sprite.extrude.count,
    sprite.common_divisor_offset.y + sprite.extrude.count,
  };
}

std::vector<Texture> pack_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    prepare_sprite(sprite);

  auto textures = pack_sprites_by_output(sprites);

  // check if texture filename indices exceed sequence length
  for (const auto& texture : textures)
    if (texture.filename_index >= texture.output->filename.count())
      throw_not_all_sprites_packed();

  for (auto& sprite : sprites)
    complete_sprite(sprite);

  return textures;
}

void create_textures_from_filename_indices(const OutputPtr& output_ptr, 
    SpriteSpan sprites, std::vector<Texture>& textures) {

  // sort sprites by texture index
  std::sort(std::begin(sprites), std::end(sprites),
    [](const Sprite& a, const Sprite& b) {
      return std::tie(a.texture_filename_index, a.index) <
             std::tie(b.texture_filename_index, b.index);
    });

  // create textures
  auto texture_begin = sprites.begin();
  const auto end = sprites.end();
  for (auto it = texture_begin;; ++it)
    if (it == end || 
        it->texture_filename_index != texture_begin->texture_filename_index) {
      textures.push_back({ 
        output_ptr, 
        texture_begin->texture_filename_index, 
        0, 0, 
        SpriteSpan(texture_begin, it)
      });

      texture_begin = it;
      if (it == end)
        break;
    }
}

void recompute_texture_size(Texture& texture) {
  const auto& output = *texture.output;

  auto max_x = 0;
  auto max_y = 0;
  for (const auto& sprite : texture.sprites) {
    max_x = std::max(max_x, sprite.trimmed_rect.x + 
      (sprite.rotated ? sprite.trimmed_rect.h : sprite.trimmed_rect.w));
    max_y = std::max(max_y, sprite.trimmed_rect.y + 
      (sprite.rotated ? sprite.trimmed_rect.w : sprite.trimmed_rect.h));
  }
  texture.width = max_x + output.border_padding;
  texture.height = max_y + output.border_padding;

  if (output.align_width)
    texture.width = ceil(texture.width, output.align_width);

  if (output.power_of_two) {
    texture.width = ceil_to_pot(texture.width);
    texture.height = ceil_to_pot(texture.height);
  }
  if (output.square)
    texture.width = texture.height = std::max(texture.width, texture.height);
}

[[noreturn]] void throw_not_all_sprites_packed() {
  throw std::runtime_error("not all sprites could be packed");
}

} // namespace
