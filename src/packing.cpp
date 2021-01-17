
#include "packing.h"
#include "stb/stb_rect_pack.h"
#include "image.h"
#include <cmath>
#include <algorithm>
#include <span>

namespace {
  void pack_sprite_texture(const Settings& settings, std::span<Sprite> sprites) {
    const auto& texture = *sprites.front().texture;

    auto width = (texture.width ? texture.width : 1024);
    auto height = (texture.height ? texture.height : 65535);
    auto context = stbrp_context{ };
    auto nodes = std::vector<stbrp_node>(static_cast<size_t>(width));
    stbrp_init_target(&context, width, height, nodes.data(), static_cast<int>(nodes.size()));

    // sort sprites by margin
    std::stable_sort(begin(sprites), end(sprites),
      [](const auto& a, const auto& b) { return (a.margin < b.margin); });

    auto rects = std::vector<stbrp_rect>();
    rects.reserve(sprites.size());
    auto offset = size_t{ };
    for (auto i = 0u; i < sprites.size(); ++i) {
      const auto& sprite = sprites[i];
      rects.push_back({
        static_cast<int>(i),
        static_cast<unsigned short>(sprite.trimmed_source_rect.w + sprite.margin),
        static_cast<unsigned short>(sprite.trimmed_source_rect.h + sprite.margin),
        0, 0, 0
      });

      // pack sprites with different margins separately
      if (i == sprites.size() - 1 || sprites[i + 1].margin != sprite.margin) {
        if (!stbrp_pack_rects(&context, rects.data() + offset,
            static_cast<int>(rects.size() - offset)))
          throw std::runtime_error("packing sheet failed");
        offset = rects.size();
      }
    }

    // calculate texture height
    if (!texture.height) {
      height = 0;
      for (const auto& rect : rects)
        height = std::max(height, rect.y + rect.h);
    }

    // copy from sources to target sheet
    auto target = Image(width, height);
    for (const auto& rect : rects) {
      auto& sprite = sprites[static_cast<size_t>(rect.id)];
      sprite.trimmed_rect = {
        rect.x,
        rect.y,
        rect.w - sprite.margin,
        rect.h - sprite.margin
      };
      copy_rect(*sprite.source, sprite.trimmed_source_rect,
        target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
    }

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

    if (settings.debug) {
      for (const auto& rect : rects) {
        auto& sprite = sprites[static_cast<size_t>(rect.id)];
        draw_rect(target, sprite.rect, RGBA{ { 255, 0, 255, 128 } });

        const auto pivot_point = Rect{
          sprite.rect.x + static_cast<int>(sprite.pivot_point.x - 0.25f),
          sprite.rect.y + static_cast<int>(sprite.pivot_point.y - 0.25f),
          (sprite.pivot_point.x == std::floor(sprite.pivot_point.x) ? 2 : 1),
          (sprite.pivot_point.y == std::floor(sprite.pivot_point.y) ? 2 : 1),
        };
        draw_rect(target, pivot_point, RGBA{ { 255, 0, 0, 255 } });
      }
    }

    // sort sprites by id
    std::sort(begin(sprites), end(sprites),
      [&](const auto& a, const auto& b) {
        return split_name_number(a.id) < split_name_number(b.id);
      });

    save_image(target, (texture.filename.empty() ?
        settings.default_texture_file : utf8_to_path(texture.filename)));
  }
} // namespace

void pack_sprite_sheet(const Settings& settings, std::vector<Sprite>& sprites) {
  // trim rects
  for (auto& sprite : sprites)
    if (sprite.trim != Trim::none) {
      sprite.trimmed_source_rect = get_used_bounds(*sprite.source, sprite.source_rect);
    }
    else {
      sprite.trimmed_source_rect = sprite.source_rect;
    }

  // sort sprites by texture
  std::stable_sort(begin(sprites), end(sprites),
    [](const auto& a, const auto& b) { return a.texture->filename < b.texture->filename; });

  // pack sprites per texture
  auto begin = sprites.begin();
  for (auto it = sprites.begin(); begin != sprites.end(); ++it) {
    if (it == sprites.end() || it->texture->filename != begin->texture->filename) {
      pack_sprite_texture(settings, { begin, it });
      begin = it;
    }
  }
}
