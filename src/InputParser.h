#pragma once

#include "Definition.h"
#include <sstream>
#include <map>

namespace spright {

class InputParser {
public:
  explicit InputParser(const Settings& settings);
  void parse(std::istream& input, const std::filesystem::path& input_file = { });
  const std::vector<Sprite>& sprites() const & { return m_sprites; }
  std::vector<Input> inputs() && { return std::move(m_inputs); }
  std::vector<Sprite> sprites() && { return std::move(m_sprites); }
  VariantMap variables() && { return std::move(m_variables); }
  std::string autocomplete_output() const { return std::move(m_autocomplete_output).str(); }
  std::string warning_output() const { return std::move(m_warning_output).str(); }

private:
  struct NotAppliedDefinition {
    Definition definition;
    int line_number;
  };

  std::shared_ptr<Sheet> get_sheet(const std::string& sheet_id);
  std::shared_ptr<Output> get_output(const std::filesystem::path& filename);
  ImagePtr get_source(const State& state);
  ImagePtr get_source(const State& state, int index);
  ImagePtr get_source(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey);
  MapVectorPtr get_maps(const State& state, const ImagePtr& source);
  void sprite_ends(State& state);
  void deduce_grid_size(State& state);
  void deduce_globbing_sources(State& state);
  void deduce_sequence_sprites(State& state);
  void deduce_grid_sprites(State& state);
  void deduce_atlas_sprites(State& state);
  void deduce_single_sprite(State& state);
  void sheet_ends(State& state);
  void output_ends(State& state);
  void input_begins(State& state);
  void input_ends(State& state);
  void source_ends(State& state);
  void scope_ends(State& state);
  void update_applied_definitions(Definition definition);
  void update_not_applied_definitions(Definition definition, int line_number);
  void check_not_applied_definitions();
  void handle_exception(std::function<void()> function);

  const Settings& m_settings;
  std::stringstream m_autocomplete_output;
  std::stringstream m_warning_output;
  std::filesystem::path m_input_file;
  int m_error_line_number{ };
  std::vector<Input> m_inputs;
  std::map<std::string, std::shared_ptr<Sheet>> m_sheets;
  std::map<std::filesystem::path, std::shared_ptr<Output>> m_outputs;
  std::map<std::filesystem::path, ImagePtr> m_sources;
  std::map<ImagePtr, MapVectorPtr> m_maps;
  std::vector<Sprite> m_sprites;
  VariantMap m_variables;
  int m_sprites_in_current_source{ };
  Point m_current_grid_cell{ };
  int m_current_sequence_index{ };
  std::vector<ImagePtr> m_current_input_sources;
  std::string m_detected_indentation;
  std::vector<std::map<Definition, NotAppliedDefinition>> m_not_applied_definitions;
};

} // namespace
