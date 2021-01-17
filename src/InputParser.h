#pragma once

#include "input.h"
#include "FilenameSequence.h"

enum class Definition {
  none,
  begin,
  path,
  sheet,
  colorkey,
  grid,
  offset,
  sprite,
  rect,
  skip,
  pivot,
  tag,
  margin,
  trim,
};

struct State {
  Definition definition{ };
  int level;
  std::string indent;

  std::filesystem::path path;
  FilenameSequence sheet;
  RGBA colorkey{ };
  std::map<std::string, std::string> tags;
  std::string sprite;
  Size grid{ };
  Pivot pivot{ PivotX::center, PivotY::middle };
  PointF pivot_point{ };
  Rect rect{ };
  int margin{ };
  Trim trim{ };
};

class InputParser {
public:
  explicit InputParser(const Settings& settings);
  void parse_autocomplete();
  void parse(std::istream& input);
  const std::vector<Sprite>& sprites() const & { return m_sprites; }
  std::vector<Sprite> sprites() && { return m_sprites; }

private:
  [[noreturn]] void error(std::string message);
  void check(bool condition, std::string_view message);
  ImagePtr get_sheet(const State& state);
  ImagePtr get_sheet(const std::filesystem::path& full_path, RGBA colorkey);
  void sprite_ends(State& state);
  void autocomplete_sequence_sprites(State& state);
  void autocomplete_grid_sprites(State& state);
  void autocomplete_unaligned_sprites(State& state);
  void sheet_ends(State& state);
  void apply_definition(State& state,
      Definition definition,
      std::vector<std::string_view>& arguments);
  bool has_implicit_scope(Definition definition);
  void scope_ends(State& state);

  const Settings& m_settings;
  std::map<std::filesystem::path, ImagePtr> m_sheets;
  std::stringstream m_autocomplete_output;
  int m_line_number{ };
  std::vector<Sprite> m_sprites;
  int m_sprites_in_current_sheet{ };
  Point m_current_offset{ };
  int m_current_sequence_index{ };
};
