#pragma once

#include "Definition.h"
#include <sstream>
#include <map>

namespace spright {

class InputParser {
public:
  explicit InputParser(const Settings& settings);
  void parse(std::istream& input);
  const std::vector<Sprite>& sprites() const & { return m_sprites; }
  std::vector<Sprite> sprites() && { return std::move(m_sprites); }
  std::string autocomplete_output() const { return m_autocomplete_output.str(); }

private:
  struct NotAppliedDefinition {
    Definition definition;
    int line_number;
  };

  std::string get_sprite_id(const State& state) const;
  OutputPtr get_output(const State& state);
  ImagePtr get_sheet(const State& state);
  ImagePtr get_sheet(const State& state, int index);
  ImagePtr get_sheet(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey);
  LayerVectorPtr get_layers(const State& state, const ImagePtr& sheet);
  void sprite_ends(State& state);
  void deduce_grid_size(State& state);
  void deduce_globbing_sheets(State& state);
  void deduce_sequence_sprites(State& state);
  void deduce_grid_sprites(State& state);
  void deduce_atlas_sprites(State& state);
  void deduce_single_sprite(State& state);
  void output_ends(State& state);
  void sheet_ends(State& state);
  void scope_ends(State& state);
  void update_applied_definitions(Definition definition);
  void update_not_applied_definitions(Definition definition);
  void check_not_applied_definitions();

  const Settings& m_settings;
  std::stringstream m_autocomplete_output;
  int m_line_number{ };
  std::map<std::filesystem::path, OutputPtr> m_outputs;
  std::map<std::filesystem::path, ImagePtr> m_sheets;
  std::map<ImagePtr, LayerVectorPtr> m_layers;
  std::vector<Sprite> m_sprites;
  int m_sprites_in_current_sheet{ };
  std::string m_detected_indentation;
  std::vector<std::map<Definition, NotAppliedDefinition>> m_not_applied_definitions;
};

} // namespace
