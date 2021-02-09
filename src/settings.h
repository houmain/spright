#pragma once

#include <filesystem>
#include <vector>

struct Settings {
  std::filesystem::path input_file{ "spright.conf" };
  std::filesystem::path output_file{ "spright.txt" };
  std::filesystem::path template_file;
  bool autocomplete{ };
  bool debug{ };
};

bool interpret_commandline(Settings& settings, int argc, const char* argv[]);
void print_help_message(const char* argv0);
