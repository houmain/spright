#pragma once

#include "input.h"
#include "FilenameSequence.h"
#include <sstream>

enum class Definition {
  none,
  group,

  texture,
  width,
  height,
  max_width,
  max_height,
  power_of_two,
  square,
  align_width,
  allow_rotate,
  padding,
  deduplicate,
  alpha,

  path,
  sheet,
  colorkey,
  grid,
  grid_offset,
  grid_spacing,
  row,
  skip,
  span,

  sprite,
  id,
  rect,
  pivot,
  tag,
  trim,
  trim_threshold,
  trim_margin,
  crop,
  extrude,
  common_divisor,
};

struct State {
  Definition definition{ };
  int level;
  std::string indent;
  std::string detected_indentation;

  std::filesystem::path texture;
  int width{ };
  int height{};
  int max_width{ };
  int max_height{};
  bool power_of_two{ };
  bool square{ };
  int align_width{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  bool deduplicate{ };
  Alpha alpha{ };
  RGBA alpha_colorkey{ };

  std::filesystem::path path;
  FilenameSequence sheet;
  RGBA colorkey{ };
  std::map<std::string, std::string> tags;
  std::string sprite_id;
  Size grid{ };
  Size grid_offset{ };
  Size grid_spacing{ };
  Size span{ 1, 1 };
  Pivot pivot{ PivotX::center, PivotY::middle };
  PointF pivot_point{ };
  Rect rect{ };
  Trim trim{ };
  bool crop{ };
  int trim_threshold{ 1 };
  int trim_margin{ };
  int extrude{ };
  Size common_divisor{ 1, 1 };
};

class InputParser {
public:
  explicit InputParser(const Settings& settings);
  void parse(std::istream& input);
  const std::vector<Sprite>& sprites() const & { return m_sprites; }
  std::vector<Sprite> sprites() && { return std::move(m_sprites); }
  std::string autocomplete_output() const { return m_autocomplete_output.str(); }

private:
  [[noreturn]] void error(std::string message);
  void check(bool condition, std::string_view message);
  std::string get_sprite_id(const State& state) const;
  TexturePtr get_texture(const State& state);
  ImagePtr get_sheet(const State& state);
  ImagePtr get_sheet(const State& state, int index);
  ImagePtr get_sheet(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey);
  void sprite_ends(State& state);
  void deduce_globbing_sheets(State& state);
  void deduce_sequence_sprites(State& state);
  void deduce_grid_sprites(State& state);
  void deduce_unaligned_sprites(State& state);
  void texture_ends(State& state);
  void sheet_ends(State& state);
  void apply_definition(State& state,
      Definition definition,
      std::vector<std::string_view>& arguments);
  bool has_implicit_scope(Definition definition);
  void scope_ends(State& state);
  void validate_sprite(const Sprite& sprite);

  const Settings& m_settings;
  std::stringstream m_autocomplete_output;
  int m_line_number{ };
  std::map<std::filesystem::path, TexturePtr> m_textures;
  std::map<std::filesystem::path, ImagePtr> m_sheets;
  std::vector<Sprite> m_sprites;
  int m_sprites_in_current_sheet{ };
  int m_current_grid_cell_x{ };
  int m_current_grid_cell_y{ };
  int m_current_sequence_index{ };
};
