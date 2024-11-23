#pragma once

#include "input.h"

namespace spright {

using SpriteSpan = span<Sprite>;

struct Slice {
  SheetPtr sheet;
  int sheet_index{ };
  SpriteSpan sprites;
  int width{ };
  int height{ };
  int index{ };
  bool layered{ };

  std::optional<std::filesystem::file_time_type> last_source_written_time;
};

std::pair<int, int> get_slice_max_size(const Sheet& sheet);
void create_slices_from_indices(const SheetPtr& sheet_ptr, 
    SpriteSpan sprites, std::vector<Slice>& slices);
void recompute_slice_size(Slice& slice);
void update_last_source_written_times(std::vector<Slice>& slices);

std::vector<Slice> pack_sprites(std::vector<Sprite>& sprites);

void pack_binpack(const SheetPtr& sheet, SpriteSpan sprites,
  std::vector<Slice>& slices, bool fast);
void pack_compact(const SheetPtr& sheet, SpriteSpan sprites,
  std::vector<Slice>& slices);
void pack_single(const SheetPtr& sheet, SpriteSpan sprites,
  std::vector<Slice>& slices);
void pack_keep(const SheetPtr& sheet, SpriteSpan sprites,
  std::vector<Slice>& slices);
void pack_lines(const SheetPtr& sheet, SpriteSpan sprites, 
  std::vector<Slice>& slices, bool horizontal);
void pack_origin(const SheetPtr& sheet,  SpriteSpan sprites, 
  std::vector<Slice>& slices, bool layered);

} // namespace
