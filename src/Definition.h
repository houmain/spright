#pragma once

#include "input.h"
#include "FilenameSequence.h"

namespace spright {

enum class Definition {
  none,
  set,
  group,

  sheet,
  output,
  width,
  height,
  max_width,
  max_height,
  power_of_two,
  square,
  divisible_width,
  allow_rotate,
  padding,
  duplicates,
  alpha,
  pack,
  scale,
  debug,

  path,
  glob,
  input,
  colorkey,
  grid,
  grid_cells,
  grid_offset,
  grid_spacing,
  row,
  skip,
  span,
  atlas,
  maps,

  sprite,
  id,
  rect,
  pivot,
  tag,
  data,
  trim,
  trim_threshold,
  trim_margin,
  trim_channel,
  crop,
  crop_pivot,
  extrude,
  min_bounds,
  divisible_bounds,
  common_bounds,
  align,
  align_pivot,

  description,
  template_,

  MAX
};

struct State {
  Definition definition{ };
  int level{ };
  std::string indent;

  std::string sheet_id;
  std::vector<std::filesystem::path> output_filenames;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool square{ };
  int divisible_width{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  Duplicates duplicates{ };
  Alpha alpha{ };
  RGBA alpha_colorkey{ };
  Pack pack{ };
  real scale{ 1.0 };
  ResizeFilter scale_filter{ };
  bool debug{ };

  std::filesystem::path path;
  std::string glob_pattern;
  FilenameSequence source_filenames;
  std::string default_map_suffix;
  std::vector<std::string> map_suffixes;
  RGBA colorkey{ };
  StringMap tags;
  VariantMap data;
  std::string sprite_id;
  int skip_sprites{ };
  Size grid{ };
  Size grid_cells{ };
  Size grid_offset{ };
  Size grid_offset_bottom_right{ };
  Size grid_spacing{ };
  Size span{ 1, 1 };
  int atlas_merge_distance{ -1 };
  AnchorF pivot{ { 0, 0 }, AnchorX::center, AnchorY::middle };
  Rect rect{ };
  Trim trim{ Trim::rect };
  int trim_threshold{ 1 };
  int trim_margin{ };
  bool trim_gray_levels{ };
  bool crop{ };
  bool crop_pivot{ };
  Extrude extrude{ };
  Size min_bounds{ };
  Size divisible_bounds{ 1, 1 };
  std::string common_bounds;
  Anchor align{ { 0, 0 }, AnchorX::center, AnchorY::middle };
  std::string align_pivot;

  std::filesystem::path description_filename;
  std::filesystem::path template_filename;
};

Definition get_definition(std::string_view command);
Definition get_affected_definition(Definition definition);
std::string_view get_definition_name(Definition definition);

void apply_definition(Definition definition,
    std::vector<std::string_view>& arguments,
    State& state,
    Point& current_grid_cell,
    VariantMap& variables);

bool has_grid(const State& state);
bool has_atlas(const State& state);

} // namespace
