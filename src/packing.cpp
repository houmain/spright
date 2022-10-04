
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
    auto& pivot_point = sprite.pivot_point;

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
    sprite.rect.x -= sprite.common_divisor_offset.x;
    sprite.rect.y -= sprite.common_divisor_offset.y;
    sprite.rect.w += sprite.common_divisor_margin.x;
    sprite.rect.h += sprite.common_divisor_margin.y;

    switch (sprite.pivot.x) {
      case PivotX::left: pivot_point.x += 0; break;
      case PivotX::center: pivot_point.x += static_cast<float>(rect.w) / 2; break;
      case PivotX::right: pivot_point.x += static_cast<float>(rect.w); break;
    }
    switch (sprite.pivot.y) {
      case PivotY::top: pivot_point.y += 0; break;
      case PivotY::middle: pivot_point.y += static_cast<float>(rect.h) / 2; break;
      case PivotY::bottom: pivot_point.y += static_cast<float>(rect.h); break;
    }

    if (sprite.rotated)
      for (auto& vertex : sprite.vertices)
        std::swap(vertex.x, vertex.y);
  }

  void pack_texture(const OutputPtr& output,
      SpriteSpan sprites, std::vector<Texture>& textures) {
    assert(!sprites.empty());

    switch (output->pack) {
      case Pack::binpack: return pack_binpack(output, sprites, sprites.size() > 1000, textures);
      case Pack::compact: return pack_compact(output, sprites, textures);
      case Pack::single: return pack_single(output, sprites, textures);
      case Pack::keep: return pack_keep(output, sprites, textures);
    }
  }

  void pack_texture_deduplicate(const OutputPtr& output,
      SpriteSpan sprites, std::vector<Texture>& textures) {
    assert(!sprites.empty());

    // sort duplicates to back
    auto unique_sprites = sprites;
    auto duplicates = std::vector<size_t>();
    for (auto i = size_t{ }; i < unique_sprites.size(); ++i) {
      for (auto j = size_t{ }; j < i; ++j)
        if (is_identical(*sprites[i].source, sprites[i].trimmed_source_rect,
                         *sprites[j].source, sprites[j].trimmed_source_rect)) {
          std::swap(sprites[i--], unique_sprites.back());
          unique_sprites = unique_sprites.first(unique_sprites.size() - 1);
          duplicates.emplace_back(j);
          break;
        }
       }

    pack_texture(output, unique_sprites, textures);

    for (auto i = size_t{ }; i < duplicates.size(); ++i) {
      auto& duplicate = sprites[sprites.size() - 1 - i];
      if (output->duplicates == Duplicates::share) {
        const auto& sprite = unique_sprites[duplicates[i]];
        duplicate.texture_index = sprite.texture_index;
        duplicate.trimmed_rect = sprite.trimmed_rect;
        duplicate.rotated = sprite.rotated;
      }
      else {
        duplicate = { };
      }
    }
  }

  void pack_sprites_by_texture(SpriteSpan sprites, std::vector<Texture>& textures) {
    if (sprites.empty())
      return;

    // sort sprites by output
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.output->filename, a.index) <
               std::tie(b.output->filename, b.index);
      });

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
    sprite.trimmed_source_rect.w + sprite.common_divisor_margin.x + sprite.extrude * 2,
    sprite.trimmed_source_rect.h + sprite.common_divisor_margin.y + sprite.extrude * 2
  };
}

Size get_sprite_indent(const Sprite& sprite) {
  return {
    sprite.common_divisor_offset.x + sprite.extrude,
    sprite.common_divisor_offset.y + sprite.extrude,
  };
}

std::vector<Texture> pack_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    prepare_sprite(sprite);

  auto textures = std::vector<Texture>();
  pack_sprites_by_texture(sprites, textures);

  for (auto& sprite : sprites)
    complete_sprite(sprite);

  return textures;
}

} // namespace
