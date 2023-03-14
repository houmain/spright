#pragma once

#include <filesystem>
#include <vector>

namespace spright {

struct Settings {
  const char* default_input_file{ "spright.conf" };
  const char* default_output_file{ "spright.json" };
  std::filesystem::path input_file;
  std::filesystem::path output_path;
  std::filesystem::path output_file;
  std::filesystem::path template_file;
  bool autocomplete{ };
  bool describe{ };
  bool rebuild{ };
  bool verbose{ };
};

bool interpret_commandline(Settings& settings, int argc, const char* argv[]);
void print_help_message(const char* argv0);

} // namespace
