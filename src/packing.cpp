
#include "packing.h"
#include "image.h"
#include "FilenameSequence.h"
#include "texpack/packer.h"
#include <cmath>
#include <algorithm>
#include <span>

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

  std::pair<int, int> get_max_texture_size(const Texture& texture) {
    return std::make_pair(
      get_max_size(texture.width, texture.max_width, texture.power_of_two),
      get_max_size(texture.height, texture.max_height, texture.power_of_two));
  }

  bool fits_in_texture(const Sprite& sprite, int max_width, int max_height, bool allow_rotate) {
    const auto sw = sprite.trimmed_source_rect.w;
    const auto sh = sprite.trimmed_source_rect.h;
    return ((sw <= max_width && sh <= max_height) ||
            (allow_rotate && sw <= max_height && sh <= max_width));
  }

  void prepare_sprites(std::span<Sprite> sprites) {
    // trim rects
    for (auto& sprite : sprites)
      if (sprite.trim != Trim::none) {
        sprite.trimmed_source_rect =
          get_used_bounds(*sprite.source, sprite.source_rect, sprite.trim_threshold);

        if (sprite.trim_margin)
          sprite.trimmed_source_rect = intersect(expand(
            sprite.trimmed_source_rect, sprite.trim_margin), sprite.source_rect);
      }
      else {
        sprite.trimmed_source_rect = sprite.source_rect;
      }
  }

  void complete_sprite_info(std::span<Sprite> sprites) {
    // calculate rects and pivot points
    for (auto& sprite : sprites) {
      auto& rect = sprite.rect;
      auto& pivot_point = sprite.pivot_point;

      if (sprite.trim == Trim::crop) {
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
      if (sprite.integral_pivot_point) {
        pivot_point.x = std::floor(pivot_point.x);
        pivot_point.y = std::floor(pivot_point.y);
      }
      sprite.trimmed_pivot_point.x =
        pivot_point.x + static_cast<float>(sprite.rect.x - sprite.trimmed_rect.x);
      sprite.trimmed_pivot_point.y =
        pivot_point.y + static_cast<float>(sprite.rect.y - sprite.trimmed_rect.y);
    }
  }

  void pack_sprite_texture(const Texture& texture,
      std::span<Sprite> sprites, std::vector<PackedTexture>& packed_textures) {
    if (sprites.empty())
      return;

    const auto [pack_width, pack_height] = get_max_texture_size(texture);
    for (const auto& sprite : sprites)
      if (!fits_in_texture(sprite,
              pack_width - texture.border_padding * 2,
              pack_height - texture.border_padding * 2,
              texture.allow_rotate))
        throw std::runtime_error("sprite '" + sprite.id + "' can not fit in texture");

    // pack rects
    auto pkr_sprites = std::vector<pkr::Sprite>();
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

      const auto& sprite = sprites[i];
      if (!is_duplicate)
        pkr_sprites.push_back({
          static_cast<int>(i),
          0, 0,
          sprite.trimmed_source_rect.w + texture.shape_padding + sprite.extrude * 2,
          sprite.trimmed_source_rect.h + texture.shape_padding + sprite.extrude * 2,
          false
        });
    }

    const auto pack_max_size = (pack_width > texture.width);
    auto pkr_sheets = pkr::pack(
      pkr::Params{
        texture.power_of_two,
        texture.allow_rotate,
        texture.border_padding * 2,
        pack_width,
        pack_height,
        pack_max_size,
      },
      pkr_sprites);

    if (std::cmp_greater(pkr_sheets.size(), texture.filename.count()))
      throw std::runtime_error("not all sprites fit on texture '" +
        texture.filename.filename() + "'");

    // update sprite rects
    auto texture_index = 0;
    for (const auto& pkr_sheet : pkr_sheets) {
      for (const auto& pkr_sprite : pkr_sheet.sprites) {
        auto& sprite = sprites[static_cast<size_t>(pkr_sprite.id)];
        sprite.rotated = pkr_sprite.rotated;
        sprite.texture_index = texture_index;
        sprite.trimmed_rect = {
          pkr_sprite.x + sprite.extrude - texture.border_padding,
          pkr_sprite.y + sprite.extrude - texture.border_padding,
          pkr_sprite.width - sprite.extrude * 2 - texture.shape_padding,
          pkr_sprite.height - sprite.extrude * 2 - texture.shape_padding
        };
      }
      ++texture_index;
    }

    for (auto [i, j] : duplicates) {
      sprites[i].rotated = sprites[j].rotated;
      sprites[i].texture_index = sprites[j].texture_index;
      sprites[i].trimmed_rect = sprites[j].trimmed_rect;
    }

    complete_sprite_info(sprites);

    // sort sprites by sheet index
    if (pkr_sheets.size() > 1)
      std::sort(begin(sprites), end(sprites),
        [](const Sprite& a, const Sprite& b) { return a.texture_index < b.texture_index; });

    // add to output textures
    auto texture_begin = sprites.begin();
    const auto end = sprites.end();
    for (auto it = texture_begin;; ++it)
      if (it == end || it->texture_index != texture_begin->texture_index) {
        const auto sheet_index = texture_begin->texture_index;
        const auto sheet_sprites = std::span(texture_begin, it);

        // calculate texture dimensions
        auto width = texture.width;
        auto height = texture.height;
        for (const auto& sprite : sheet_sprites) {
          width = std::max(width, sprite.trimmed_rect.x +
            (sprite.rotated ? sprite.trimmed_rect.h : sprite.trimmed_rect.w) +
            sprite.extrude + texture.border_padding);
          height = std::max(height, sprite.trimmed_rect.y +
            (sprite.rotated ? sprite.trimmed_rect.w : sprite.trimmed_rect.h) +
            sprite.extrude + texture.border_padding);
        }
        if (texture.power_of_two) {
          width = ceil_to_pot(width);
          height = ceil_to_pot(height);
        }

        packed_textures.push_back({
          texture.filename.get_nth_filename(sheet_index),
          width,
          height,
          sheet_sprites,
          texture.alpha,
          texture.colorkey,
        });

        texture_begin = it;
        if (it == end)
          break;
      }
  }

  void pack_sprites_by_texture(std::span<Sprite> sprites, std::vector<PackedTexture>& packed_textures) {
    // sort sprites by texture
    std::stable_sort(begin(sprites), end(sprites),
      [](const Sprite& a, const Sprite& b) { return a.texture->filename < b.texture->filename; });

    auto begin = sprites.begin();
    for (auto it = sprites.begin(); begin != sprites.end(); ++it) {
      if (it == sprites.end() ||
          it->texture->filename != begin->texture->filename) {
        pack_sprite_texture(*begin->texture, { begin, it }, packed_textures);
        begin = it;
      }
    }
  }
} // namespace

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites) {
  auto packed_textures = std::vector<PackedTexture>();
  prepare_sprites(sprites);
  pack_sprites_by_texture(sprites, packed_textures);
  return packed_textures;
}
