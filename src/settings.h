#pragma once

#include <filesystem>
#include <vector>

struct Settings {
  std::filesystem::path input_file;
  std::filesystem::path output_file{ "spright.txt" };
  std::filesystem::path sheet_file{ "spright.png" };
  std::filesystem::path template_file;
  bool autocomplete{ };
  int sheet_width{ 256 };
  int sheet_height{ };
  bool debug{ };
};

bool interpret_commandline(Settings& settings, int argc, const char* argv[]);
void print_help_message(const char* argv0);
