#pragma once

#include <filesystem>
#include <vector>

struct Settings {
  const char* default_input_file{ "spright.conf" };
  const char* default_output_file{ "spright.json" };
  std::vector<std::filesystem::path> input_files;
  std::string input;
  std::filesystem::path output_path;
  std::filesystem::path output_file;
  std::filesystem::path template_file;
  bool autocomplete{ };
  bool debug{ };
};

bool interpret_commandline(Settings& settings, int argc, const char* argv[]);
void print_help_message(const char* argv0);
