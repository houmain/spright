#pragma once

#include <filesystem>
#include <vector>

namespace spright {

enum class Mode {
  update,
  rebuild,
  autocomplete,
  describe,
  describe_input,
};

struct Settings {
  Mode mode{ Mode::update };
  const char* default_input_file{ "spright.conf" };
  const char* default_output_file{ "spright.json" };
  std::filesystem::path input_file;
  std::filesystem::path output_path;
  std::filesystem::path output_file;
  std::filesystem::path template_file;
  std::string autocomplete_pattern;
  bool verbose{ };
};

bool interpret_commandline(Settings& settings, int argc, const char* argv[]);
void print_help_message(const char* argv0);

} // namespace
