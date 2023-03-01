
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

  void update_sprite_size(Sprite& s) {
    const auto size = s.trimmed_source_rect.size();
    s.size.x = std::max(s.min_size.x,
      ceil(size.x + 2 * s.extrude.count, s.divisible_size.x));
    s.size.y = std::max(s.min_size.y,
      ceil(size.y + 2 * s.extrude.count, s.divisible_size.y));
  }

  void update_sprite_offset(Sprite& s) {
    const auto margin = s.size - s.trimmed_source_rect.size();
    switch (s.align.x) {
      case AlignX::left:   s.offset.x = 0; break;
      case AlignX::center: s.offset.x = margin.x / 2; break;
      case AlignX::right:  s.offset.x = margin.x; break;
    }
    switch (s.align.y) {
      case AlignY::top:    s.offset.y = 0; break;
      case AlignY::middle: s.offset.y = margin.y / 2; break;
      case AlignY::bottom: s.offset.y = margin.y; break;
    }
  }

  void update_sprite_rect(Sprite& s) {
    s.trimmed_rect.w = s.trimmed_source_rect.w;
    s.trimmed_rect.h = s.trimmed_source_rect.h;

    if (s.crop) {
      s.rect = s.trimmed_rect;
    }
    else {
      s.rect = {
        s.trimmed_rect.x - (s.trimmed_source_rect.x - s.source_rect.x),
        s.trimmed_rect.y - (s.trimmed_source_rect.y - s.source_rect.y),
        s.source_rect.w,
        s.source_rect.h,
      };
    }
  }

  void update_sprite_pivot_point(Sprite &s) {
    const auto pivot_rect = RectF(s.crop_pivot ?
      s.trimmed_source_rect : s.source_rect);
    switch (s.pivot.x) {
      case PivotX::left:   s.pivot_point.x += 0; break;
      case PivotX::center: s.pivot_point.x += pivot_rect.w / 2; break;
      case PivotX::right:  s.pivot_point.x += pivot_rect.w; break;
    }
    switch (s.pivot.y) {
      case PivotY::top:    s.pivot_point.y += 0; break;
      case PivotY::middle: s.pivot_point.y += pivot_rect.h / 2; break;
      case PivotY::bottom: s.pivot_point.y += pivot_rect.h; break;
    }
    s.pivot_point.x -= s.rect.x - s.trimmed_rect.x;
    s.pivot_point.y -= s.rect.y - s.trimmed_rect.y;
    s.pivot_point.x += (pivot_rect.x - s.trimmed_source_rect.x);
    s.pivot_point.y += (pivot_rect.y - s.trimmed_source_rect.y);
  }

  void pack_slice(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
    assert(!sprites.empty());

    switch (sheet->pack) {
      case Pack::binpack: return pack_binpack(sheet, sprites, sprites.size() > 1000, slices);
      case Pack::compact: return pack_compact(sheet, sprites, slices);
      case Pack::single: return pack_single(sheet, sprites, slices);
      case Pack::keep: return pack_keep(sheet, sprites, slices);
      case Pack::rows: return pack_lines(true, sheet, sprites, slices);
      case Pack::columns: return pack_lines(false, sheet, sprites, slices);
      case Pack::layers: return pack_layers(sheet, sprites, slices);
    }
  }

  void pack_slice_deduplicate(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
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

    pack_slice(sheet, unique_sprites, slices);

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
        duplicate.slice_index = sprite.slice_index;
        duplicate.trimmed_rect = sprite.trimmed_rect;
        duplicate.rotated = sprite.rotated;
      }
    }
  }

  std::vector<Slice> pack_sprites_by_sheet(SpriteSpan sprites) {
    if (sprites.empty())
      return { };

    // sort sprites by sheet
    std::sort(std::begin(sprites), std::end(sprites),
      [](const Sprite& a, const Sprite& b) {
        return std::tie(a.sheet->index, a.index) <
               std::tie(b.sheet->index, b.index);
      });

    auto slices = std::vector<Slice>();
    for (auto begin = sprites.begin(), it = begin; ; ++it)
      if (it == sprites.end() ||
          it->sheet != begin->sheet) {
        const auto& sheet = begin->sheet;
        if (sheet->duplicates != Duplicates::keep)
          pack_slice_deduplicate(sheet, { begin, it }, slices);
        else
          pack_slice(sheet, { begin, it }, slices);

        if (it == sprites.end())
          break;
        begin = it;
      }
    return slices;
  }

  void update_common_sizes(std::vector<Sprite>& sprites) {
    auto sprites_by_key = std::map<std::string, std::vector<Sprite*>>();
    for (auto& sprite : sprites)
      if (!sprite.common_size.empty())
        sprites_by_key[sprite.common_size].push_back(&sprite);

    for (const auto& [key, sprites] : sprites_by_key) {
      auto max_size = Size{ };
      for (const auto* sprite : sprites) {
        max_size.x = std::max(max_size.x, sprite->size.x);
        max_size.y = std::max(max_size.y, sprite->size.y);
      }
      for (auto* sprite : sprites) {
        sprite->size.x = std::max(max_size.x, sprite->size.x);
        sprite->size.y = std::max(max_size.y, sprite->size.y);
      }
    }
  }
} // namespace

std::pair<int, int> get_slice_max_size(const Sheet& sheet) {
  return {
    get_max_size(sheet.width, sheet.max_width, sheet.power_of_two),
    get_max_size(sheet.height, sheet.max_height, sheet.power_of_two)
  };
}

std::vector<Slice> pack_sprites(std::vector<Sprite>& sprites) {
  for (auto& sprite : sprites)
    update_sprite_size(sprite);
  update_common_sizes(sprites);
  for (auto& sprite : sprites)
    update_sprite_offset(sprite);

  auto slices = pack_sprites_by_sheet(sprites);

  for (auto& sprite : sprites) {
    update_sprite_rect(sprite);
    update_sprite_pivot_point(sprite);
  }

  // finish slices
  for (auto i = size_t{ }; i < slices.size(); ++i) {
    auto& slice = slices[i];
    recompute_slice_size(slice);
    slice.index = to_int(i);
  }
  return slices;
}

void create_slices_from_indices(const SheetPtr& sheet_ptr, 
    SpriteSpan sprites, std::vector<Slice>& slices) {

  // sort sprites by slice index
  std::sort(std::begin(sprites), std::end(sprites),
    [](const Sprite& a, const Sprite& b) {
      return std::tie(a.slice_index, a.index) <
             std::tie(b.slice_index, b.index);
    });

  // create slices
  auto begin = sprites.begin();
  const auto end = sprites.end();
  for (auto it = begin;; ++it)
    if (it == end || 
        it->slice_index != begin->slice_index) {
      slices.push_back({ 
        sheet_ptr,
        begin->slice_index,
        SpriteSpan(begin, it),
      });

      begin = it;
      if (it == end)
        break;
    }
}

void recompute_slice_size(Slice& slice) {
  const auto& sheet = *slice.sheet;

  auto max_x = 0;
  auto max_y = 0;
  for (const auto& sprite : slice.sprites) {
    max_x = std::max(max_x, sprite.trimmed_rect.x - sprite.offset.x +
      (sprite.rotated ? sprite.size.y : sprite.size.x));
    max_y = std::max(max_y, sprite.trimmed_rect.y - sprite.offset.y +
      (sprite.rotated ? sprite.size.x : sprite.size.y));
  }
  slice.width = std::max(sheet.width, max_x + sheet.border_padding);
  slice.height = std::max(sheet.height, max_y + sheet.border_padding);

  if (sheet.divisible_width)
    slice.width = ceil(slice.width, sheet.divisible_width);

  if (sheet.power_of_two) {
    slice.width = ceil_to_pot(slice.width);
    slice.height = ceil_to_pot(slice.height);
  }
  if (sheet.square)
    slice.width = slice.height = std::max(slice.width, slice.height);
}

[[noreturn]] void throw_not_all_sprites_packed() {
  throw std::runtime_error("not all sprites could be packed");
}

void update_last_source_written_times(std::vector<Slice>& slices) {
  scheduler.for_each_parallel(slices,
    [](Slice& slice) {
      auto last_write_time = get_last_write_time(slice.sheet->input_file);

      auto sources = std::unordered_set<const Image*>();
      for (const auto& sprite : slice.sprites)
        sources.insert(sprite.source.get());
      for (const auto& source : sources)
        last_write_time = std::max(last_write_time,
          get_last_write_time(source->path() / source->filename()));

      slice.last_source_written_time = last_write_time;
    });
}

} // namespace
