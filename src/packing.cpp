
#include "packing.h"
#include "image.h"
#include "FilenameSequence.h"
#include "rect_pack/rect_pack.h"
#include <cmath>
#include <array>
#include <algorithm>

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

  std::pair<int, int> get_texture_max_size(const Texture& texture) {
    return {
      get_max_size(texture.width, texture.max_width, texture.power_of_two),
      get_max_size(texture.height, texture.max_height, texture.power_of_two)
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
      case PivotX::left: pivot_point.x = 0; break;
      case PivotX::center: pivot_point.x = static_cast<float>(rect.w) / 2; break;
      case PivotX::right: pivot_point.x = static_cast<float>(rect.w); break;
      case PivotX::custom: pivot_point.x = sprite.pivot_point.x;
    }
    switch (sprite.pivot.y) {
      case PivotY::top: pivot_point.y = 0; break;
      case PivotY::middle: pivot_point.y = static_cast<float>(rect.h) / 2; break;
      case PivotY::bottom: pivot_point.y = static_cast<float>(rect.h); break;
      case PivotY::custom: pivot_point.y = sprite.pivot_point.y;
    }

    if (sprite.rotated)
      for (auto& vertex : sprite.vertices)
        std::swap(vertex.x, vertex.y);
  }

  void pack_sprite_texture(const Texture& texture,
      std::span<Sprite> sprites, std::vector<PackedTexture>& packed_textures) {
    assert(!sprites.empty());

    for (auto& sprite : sprites)
      prepare_sprite(sprite);

    // pack rects
    auto pack_sizes = std::vector<rect_pack::Size>();
    pack_sizes.reserve(sprites.size());
    auto duplicates = std::vector<std::pair<size_t, size_t>>();
    for (auto i = size_t{ }; i < sprites.size(); ++i) {
      auto is_duplicate = false;
      if (texture.deduplicate)
        for (auto j = size_t{ }; j < i; ++j)
          if (is_identical(*sprites[i].source, sprites[i].trimmed_source_rect,
                           *sprites[j].source, sprites[j].trimmed_source_rect)) {
            duplicates.emplace_back(i, j);
            is_duplicate = true;
          }

      if (!is_duplicate) {
        auto size = get_sprite_size(sprites[i]);
        size.x += texture.shape_padding;
        size.y += texture.shape_padding;

        pack_sizes.push_back({ static_cast<int>(i), size.x, size.y });
      }
    }

    const auto [max_texture_width, max_texture_height] = get_texture_max_size(texture);
    auto pack_sheets = pack(
      rect_pack::Settings{
        .method = (sprites.size() <= 1000 ? rect_pack::Method::Best : rect_pack::Method::Best_Skyline),
        .max_sheets = texture.filename.count(),
        .power_of_two = texture.power_of_two,
        .square = texture.square,
        .allow_rotate = texture.allow_rotate,
        .align_width = texture.align_width,
        .border_padding = texture.border_padding,
        .over_allocate = texture.shape_padding,
        .min_width = texture.width,
        .min_height = texture.height,
        .max_width = max_texture_width,
        .max_height = max_texture_height,
      },
      std::move(pack_sizes));

    // update sprite rects
    auto packed_sprites = 0;
    auto texture_index = 0;
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

    for (auto [i, j] : duplicates)
      if (!empty(sprites[j].trimmed_rect)) {
        sprites[i].rotated = sprites[j].rotated;
        sprites[i].texture_index = sprites[j].texture_index;
        sprites[i].trimmed_rect = sprites[j].trimmed_rect;
        ++packed_sprites;
      }

    if (std::cmp_less(packed_sprites, sprites.size()))
      throw std::runtime_error("not all sprites could be packed");

    for (auto& sprite : sprites)
      complete_sprite(sprite);

    // sort sprites by texture index
    if (pack_sheets.size() > 1)
      std::sort(begin(sprites), end(sprites),
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
        const auto sheet_sprites = std::span(texture_begin, it);
        const auto& pack_sheet = pack_sheets[static_cast<size_t>(sheet_index)];
        packed_textures.push_back(PackedTexture{
          .filename = texture.filename.get_nth_filename(sheet_index),
          .width = pack_sheet.width,
          .height = pack_sheet.height,
          .sprites = sheet_sprites,
          .alpha = texture.alpha,
          .colorkey = texture.colorkey,
          .border_padding = texture.border_padding,
          .shape_padding = texture.shape_padding
        });

        texture_begin = it;
        if (it == end)
          break;
      }
  }

  void pack_sprites_by_texture(std::span<Sprite> sprites, std::vector<PackedTexture>& packed_textures) {
    if (sprites.empty())
      return;

    // sort sprites by texture
    std::sort(begin(sprites), end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.texture->filename, a.index) <
               std::tie(b.texture->filename, b.index);
      });

    for (auto begin = sprites.begin(), it = begin; ; ++it)
      if (it == sprites.end() ||
          it->texture->filename != begin->texture->filename) {
        pack_sprite_texture(*begin->texture, { begin, it }, packed_textures);
        if (it == sprites.end())
          break;
        begin = it;
      }
  }
} // namespace

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites) {
  auto packed_textures = std::vector<PackedTexture>();
  pack_sprites_by_texture(sprites, packed_textures);
  return packed_textures;
}
