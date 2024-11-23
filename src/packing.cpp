
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

  void update_sprite_bounds(Sprite& s) {
    const auto size = s.trimmed_source_rect.size();
    s.bounds.x = std::max(s.min_bounds.x,
      ceil(size.x + 2 * s.extrude.count, s.divisible_bounds.x));
    s.bounds.y = std::max(s.min_bounds.y,
      ceil(size.y + 2 * s.extrude.count, s.divisible_bounds.y));
  }

  void update_sprite_alignment(Sprite& s) {
    const auto margin = s.bounds - s.trimmed_source_rect.size();
    switch (s.align.anchor_x) {
      case AnchorX::left:   s.align.x += 0; break;
      case AnchorX::center: s.align.x += margin.x / 2; break;
      case AnchorX::right:  s.align.x += margin.x; break;
    }
    switch (s.align.anchor_y) {
      case AnchorY::top:    s.align.y += 0; break;
      case AnchorY::middle: s.align.y += margin.y / 2; break;
      case AnchorY::bottom: s.align.y += margin.y; break;
    }

    // clamp to zero
    s.align.x = std::max(s.align.x, 0);
    s.align.y = std::max(s.align.y, 0);

    // expand bounds by over-alignment
    s.bounds.x = std::max(s.bounds.x, s.trimmed_source_rect.w + s.align.x);
    s.bounds.y = std::max(s.bounds.y, s.trimmed_source_rect.h + s.align.y);
  }

  void update_aligned_pivot(std::vector<Sprite>& sprites) {
    auto sprites_by_key = std::map<std::string, std::vector<Sprite*>>();
    for (auto& sprite : sprites)
      if (!sprite.align_pivot.empty())
        sprites_by_key[sprite.align_pivot].push_back(&sprite);

    for (const auto& [key, sprites] : sprites_by_key) {
      auto max_pivot = PointF{
        std::numeric_limits<real>::min(),
        std::numeric_limits<real>::min()
      };
      for (const auto* sprite : sprites) {
        max_pivot.x = std::max(max_pivot.x, sprite->pivot.x);
        max_pivot.y = std::max(max_pivot.y, sprite->pivot.y);
      }
      for (auto* sprite : sprites) {
        auto offset = Point(max_pivot - sprite->pivot);
        sprite->align.x += offset.x;
        sprite->align.y += offset.y;
        sprite->bounds.x += offset.x;
        sprite->bounds.y += offset.y;
      }
    }
  }

  void update_common_bounds(std::vector<Sprite>& sprites) {
    auto sprites_by_key = std::map<std::string, std::vector<Sprite*>>();
    for (auto& sprite : sprites)
      if (!sprite.common_bounds.empty())
        sprites_by_key[sprite.common_bounds].push_back(&sprite);

    for (const auto& [key, sprites] : sprites_by_key) {
      auto max_bounds = Size{ };
      for (const auto* sprite : sprites) {
        max_bounds.x = std::max(max_bounds.x, sprite->bounds.x);
        max_bounds.y = std::max(max_bounds.y, sprite->bounds.y);
      }
      for (auto* sprite : sprites)
        sprite->bounds = max_bounds;
    }
  }

  void update_sprite_rect(Sprite& s) {
    if (s.sheet && s.sheet->pack != Pack::keep) {
      s.trimmed_rect.x += s.align.x;
      s.trimmed_rect.y += s.align.y;
    }
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
    switch (s.pivot.anchor_x) {
      case AnchorX::left:   s.pivot.x += 0; break;
      case AnchorX::center: s.pivot.x += pivot_rect.w / 2; break;
      case AnchorX::right:  s.pivot.x += pivot_rect.w; break;
    }
    switch (s.pivot.anchor_y) {
      case AnchorY::top:    s.pivot.y += 0; break;
      case AnchorY::middle: s.pivot.y += pivot_rect.h / 2; break;
      case AnchorY::bottom: s.pivot.y += pivot_rect.h; break;
    }
    s.pivot.x -= s.rect.x - s.trimmed_rect.x;
    s.pivot.y -= s.rect.y - s.trimmed_rect.y;
    s.pivot.x += (pivot_rect.x - s.trimmed_source_rect.x);
    s.pivot.y += (pivot_rect.y - s.trimmed_source_rect.y);
  }

  void pack_slice(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
    assert(!sprites.empty());

    switch (sheet->pack) {
      case Pack::binpack: return pack_binpack(sheet, sprites, slices, sprites.size() > 1000);
      case Pack::compact: return pack_compact(sheet, sprites, slices);
      case Pack::single: return pack_single(sheet, sprites, slices);
      case Pack::keep: return pack_keep(sheet, sprites, slices);
      case Pack::rows: return pack_lines(sheet, sprites, slices, true);
      case Pack::columns: return pack_lines(sheet, sprites, slices, false);
      case Pack::origin: return pack_origin(sheet, sprites, slices, false);
      case Pack::layers: return pack_origin(sheet, sprites, slices, true);
    }
  }

  void pack_slice_deduplicate(const SheetPtr& sheet,
      SpriteSpan sprites, std::vector<Slice>& slices) {
    assert(!sprites.empty());

    // sort duplicates to back
    auto unique_sprites = sprites;
    for (auto i = sprites.size() - 1; ; --i) {
      for (auto j = size_t{ }; j < i; ++j) {
        if (is_identical(*sprites[i].source, sprites[i].trimmed_source_rect,
                         *sprites[j].source, sprites[j].trimmed_source_rect)) {
          sprites[i].duplicate_of_index = sprites[j].index;
          std::swap(sprites[i], unique_sprites.back());
          unique_sprites = unique_sprites.first(unique_sprites.size() - 1);
          break;
        }
      }
      if (i == 0)
        break;
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

  std::string get_packing_failed_reason(const Sprite& sprite, int slice_count) {
    const auto& sheet = *sprite.sheet;

    const auto [max_width, max_height] = get_slice_max_size(sheet);
    if (sprite.rect.w + sheet.border_padding > max_width)
      return "max-width exceeded";
    if (sprite.rect.h + sheet.border_padding > max_height)
      return "max-height exceeded";

    if (slice_count == get_max_slice_count(sheet)) {
      if (slice_count == 1)
        return "does not fit on single slice";
      return "limited slice count exceeded";
    }
    return "unknown reason";
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
    update_sprite_bounds(sprite);

  // apply alignments which affect bounds first
  for (auto& sprite : sprites)
    if (!sprite.align_pivot.empty())
      update_sprite_alignment(sprite);
  update_aligned_pivot(sprites);

  // otherwise apply alignments after updating bounds
  update_common_bounds(sprites);
  for (auto& sprite : sprites)
    if (sprite.align_pivot.empty())
      update_sprite_alignment(sprite);

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

  for (const auto& sprite : sprites)
    if (sprite.sheet && sprite.slice_index < 0)
      warning("packing sprite failed: " + 
        get_packing_failed_reason(sprite, to_int(slices.size())),
        sprite.warning_line_number);

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

      if (begin->slice_index >= 0)
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
    max_x = std::max(max_x, sprite.trimmed_rect.x - sprite.align.x +
      (sprite.rotated ? sprite.bounds.y : sprite.bounds.x));
    max_y = std::max(max_y, sprite.trimmed_rect.y - sprite.align.y +
      (sprite.rotated ? sprite.bounds.x : sprite.bounds.y));
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

void update_last_source_written_times(std::vector<Slice>& slices) {
  scheduler.for_each_parallel(slices,
    [](Slice& slice) {
      auto last_write_time = get_last_write_time(slice.sheet->input_file);

      auto sources = std::unordered_set<const ImageFile*>();
      for (const auto& sprite : slice.sprites)
        sources.insert(sprite.source.get());
      for (const auto& source : sources)
        last_write_time = std::max(last_write_time,
          get_last_write_time(source->path() / source->filename()));

      slice.last_source_written_time = last_write_time;
    });
}

} // namespace
