
#include "packing.h"
#include <unordered_set>

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

  void pack_texture(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Texture>& textures) {
    assert(!sprites.empty());

    switch (sheet->pack) {
      case Pack::binpack: return pack_binpack(sheet, sprites, sprites.size() > 1000, textures);
      case Pack::compact: return pack_compact(sheet, sprites, textures);
      case Pack::single: return pack_single(sheet, sprites, textures);
      case Pack::keep: return pack_keep(sheet, sprites, textures);
      case Pack::rows: return pack_lines(true, sheet, sprites, textures);
      case Pack::columns: return pack_lines(false, sheet, sprites, textures);
      case Pack::layers: return pack_layers(sheet, sprites, textures);
    }
  }

  void pack_texture_deduplicate(const SheetPtr& sheet,
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

    pack_texture(sheet, unique_sprites, textures);

    const auto duplicate_sprites = sprites.last(sprites.size() - unique_sprites.size());
    if (sheet->duplicates == Duplicates::drop) {
      for (auto& sprite : duplicate_sprites)
        sprite.sheet = { };
    }
    else {
      // copy rectangles from unique to duplicate sprites
      auto sprites_by_index = std::map<int, const Sprite*>();
      for (const auto& sprite : unique_sprites)
        sprites_by_index[sprite.index] = &sprite;
      for (auto& duplicate : duplicate_sprites) {
        const auto& sprite = *sprites_by_index.find(duplicate.duplicate_of_index)->second;
        duplicate.texture_sheet_index = sprite.texture_sheet_index;
        duplicate.trimmed_rect = sprite.trimmed_rect;
        duplicate.rotated = sprite.rotated;
      }
    }
  }

  std::vector<Texture> pack_sprites_by_sheet(SpriteSpan sprites) {
    if (sprites.empty())
      return { };

    // sort sprites by sheet
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.sheet->index, a.index) <
               std::tie(b.sheet->index, b.index);
      });

    auto textures = std::vector<Texture>();
    for (auto begin = sprites.begin(), it = begin; ; ++it)
      if (it == sprites.end() ||
          it->sheet != begin->sheet) {
        const auto& sheet = begin->sheet;
        if (sheet->duplicates != Duplicates::keep)
          pack_texture_deduplicate(sheet, { begin, it }, textures);
        else
          pack_texture(sheet, { begin, it }, textures);

        if (it == sprites.end())
          break;
        begin = it;
      }
    return textures;
  }
} // namespace

std::pair<int, int> get_texture_max_size(const Sheet& sheet) {
  return {
    get_max_size(sheet.width, sheet.max_width, sheet.power_of_two),
    get_max_size(sheet.height, sheet.max_height, sheet.power_of_two)
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

  auto textures = pack_sprites_by_sheet(sprites);

  for (auto& sprite : sprites)
    complete_sprite(sprite);

  // finish textures
  for (auto i = size_t{ }; i < textures.size(); ++i) {
    auto& texture = textures[i];
    texture.index = to_int(i);
    texture.filename = texture.output->filename.get_nth_filename(texture.output_index);
  }
  return textures;
}

void create_textures_from_filename_indices(const SheetPtr& sheet_ptr, 
    SpriteSpan sprites, std::vector<Texture>& textures) {

  // sort sprites by texture index
  std::sort(std::begin(sprites), std::end(sprites),
    [](const Sprite& a, const Sprite& b) {
      return std::tie(a.texture_sheet_index, a.index) <
             std::tie(b.texture_sheet_index, b.index);
    });

  // create textures
  auto begin = sprites.begin();
  const auto end = sprites.end();
  for (auto it = begin;; ++it)
    if (it == end || 
        it->texture_sheet_index != begin->texture_sheet_index) {
      textures.push_back({ 
        sheet_ptr,
        begin->texture_sheet_index,
        SpriteSpan(begin, it),
      });

      begin = it;
      if (it == end)
        break;
    }
}

void recompute_texture_size(Texture& texture) {
  const auto& sheet = *texture.sheet;

  auto max_x = 0;
  auto max_y = 0;
  for (const auto& sprite : texture.sprites) {
    max_x = std::max(max_x, sprite.trimmed_rect.x + 
      (sprite.rotated ? sprite.trimmed_rect.h : sprite.trimmed_rect.w));
    max_y = std::max(max_y, sprite.trimmed_rect.y + 
      (sprite.rotated ? sprite.trimmed_rect.w : sprite.trimmed_rect.h));
  }
  texture.width = max_x + sheet.border_padding;
  texture.height = max_y + sheet.border_padding;

  if (sheet.align_width)
    texture.width = ceil(texture.width, sheet.align_width);

  if (sheet.power_of_two) {
    texture.width = ceil_to_pot(texture.width);
    texture.height = ceil_to_pot(texture.height);
  }
  if (sheet.square)
    texture.width = texture.height = std::max(texture.width, texture.height);
}

[[noreturn]] void throw_not_all_sprites_packed() {
  throw std::runtime_error("not all sprites could be packed");
}

void update_last_source_written_time(Texture& texture) {
  auto last_write_time = get_last_write_time(texture.sheet->input_file);

  auto sources = std::unordered_set<const Image*>();
  for (const auto& sprite : texture.sprites)
    sources.insert(sprite.source.get());
  for (const auto& source : sources)
    last_write_time = std::max(last_write_time,
      get_last_write_time(source->path() / source->filename()));

  texture.last_source_written_time = last_write_time;
}

} // namespace
