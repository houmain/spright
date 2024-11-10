
#include "settings.h"
#include "common.h"
#include <algorithm>
#include <iostream>
#include <iterator>

namespace spright {

bool interpret_commandline(Settings& settings, int argc, const char* argv[]) {
  for (auto i = 1; i < argc; i++) {
    const auto argument = std::string_view(argv[i]);
    if (argument == "-m" || argument == "--mode") {
      if (++i >= argc)
        return false;
      const auto mode = std::string_view(argv[i]);
      if (mode == "update") 
        settings.mode = Mode::update;
      else if (mode == "rebuild")
        settings.mode = Mode::rebuild;
      else if (mode == "describe")
        settings.mode = Mode::describe;
      else if (mode == "describe-input")
        settings.mode = Mode::describe_input;
      else if (mode == "complete") {
        settings.mode = Mode::autocomplete;
        if (i + 1 < argc && *argv[i + 1] != '-')
          settings.autocomplete_pattern = unquote(argv[++i]);
      }
      else
        return false;
    }
    else if (argument == "-c" || argument == "--complete") {
      settings.mode = Mode::autocomplete;
      if (i + 1 < argc && *argv[i + 1] != '-')
        settings.autocomplete_pattern = unquote(argv[++i]);
    }
    else if (argument == "-i" || argument == "--input") {
      if (++i >= argc)
        return false;
      settings.input_file = utf8_to_path(unquote(argv[i]));
    }
    else if (argument == "-t" || argument == "--template") {
      if (++i >= argc)
        return false;
      settings.template_file = utf8_to_path(unquote(argv[i]));
    }
    else if (argument == "-o" || argument == "--output") {
      if (++i >= argc)
        return false;
      settings.output_file = utf8_to_path(unquote(argv[i]));
      settings.output_file_set = true;
    }
    else if (argument == "-p" || argument == "--path") {
      if (++i >= argc)
        return false;
      settings.output_path = utf8_to_path(unquote(argv[i]));
    }
    else if (argument == "-v" || argument == "--verbose") {
      settings.verbose = true;
    }
    else {
      return false;
    }
  }

  if (settings.input_file.empty())
    settings.input_file = settings.default_input_file;

  if (settings.output_file.empty())
    settings.output_file =
      (settings.mode != Mode::autocomplete ? settings.default_output_file :
       settings.input_file == "stdin" ? "stdout" :
       settings.input_file);

  if (settings.output_file == "stdout")
    settings.verbose = false;

  return true;
}

void print_help_message(const char* argv0) {
  auto program = std::string_view(argv0);
  if (auto i = program.rfind('/'); i != std::string::npos)
    program = program.substr(i + 1);
  if (auto i = program.rfind('.'); i != std::string::npos)
    program = program.substr(0, i);

  const auto version =
#if __has_include("_version.h")
# include "_version.h"
  " ";
#else
  "";
#endif

  std::cout <<
    "spright " << version << "(c) 2020-2024 by Albert Kalchmair\n"
    "\n"
    "Usage: " << program << " [-options]\n"
    "  -m, --mode <mode>       sets the run mode:\n"
    "     'update'             the default run mode.\n"
    "     'rebuild'            regenerate output even when input did not change.\n"
    "     'describe'           only output description, no texture files. \n"
    "     'describe-input'     only output description of input, do not pack. \n"
    "     'complete' [pattern] complete inputs (matching optional pattern).\n"
    "  -c, --complete          shortcut for '--mode complete'.\n"
    "  -i, --input <file>      input definition file (default: " << Settings::default_input_file << ").\n"
    "  -o, --output <file>     output file containing either the output\n"
    "                     description (default: " << Settings::default_output_file << ") or the\n"
    "                     completed input definition (defaults to --input).\n"
    "  -t, --template <file>   template for the output description.\n"
    "  -p, --path <path>       path to prepend to all output files.\n"
    "  -v, --verbose           enable verbose messages.\n"
    "  -h, --help              print this help.\n"
    "\n"
    "All Rights Reserved.\n"
    "This program comes with absolutely no warranty.\n"
    "See the GNU General Public License, version 3 for details.\n"
    "\n";
}

} // namespace
