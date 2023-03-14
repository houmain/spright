
#include "settings.h"
#include "common.h"
#include <algorithm>
#include <sstream>
#include <iterator>

namespace spright {

bool interpret_commandline(Settings& settings, int argc, const char* argv[]) {
  for (auto i = 1; i < argc; i++) {
    const auto argument = std::string_view(argv[i]);
    if (argument == "-i" || argument == "--input") {
      if (++i >= argc)
        return false;
      settings.input_file = std::filesystem::u8path(unquote(argv[i]));
    }
    else if (argument == "-t" || argument == "--template") {
      if (++i >= argc)
        return false;
      settings.template_file = std::filesystem::u8path(unquote(argv[i]));
    }
    else if (argument == "-o" || argument == "--output") {
      if (++i >= argc)
        return false;
      settings.output_file = std::filesystem::u8path(unquote(argv[i]));
    }
    else if (argument == "-p" || argument == "--path") {
      if (++i >= argc)
        return false;
      settings.output_path = std::filesystem::u8path(unquote(argv[i]));
    }
    else if (argument == "-a" || argument == "--autocomplete") {
      settings.autocomplete = true;
    }
    else if (argument == "-r" || argument == "--rebuild") {
      settings.rebuild = true;
    }
    else if (argument == "-d" || argument == "--debug") {
      settings.debug = true;
    }
    else {
      return false;
    }
  }

  if (settings.input_file.empty())
    settings.input_file = settings.default_input_file;

  if (settings.output_file.empty())
    settings.output_file = settings.default_output_file;

  return true;
}

void print_help_message(const char* argv0) {
  auto program = std::string(argv0);
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

  const auto defaults = Settings{ };
  printf(
    "spright %s(c) 2020-2023 by Albert Kalchmair\n"
    "\n"
    "Usage: %s [-options]\n"
    "  -i, --input <file>     input definition file (default: %s).\n"
    "  -o, --output <file>    output description file (default: %s).\n"
    "  -t, --template <file>  template for output description.\n"
    "  -p, --path <path>      path to prepend to all output files.\n"
    "  -a, --autocomplete     autocomplete input definition.\n"
    "  -r, --regenerate       generate output even when input did not change.\n"
    "  -d, --debug            draw sprite boundaries and pivot points on output.\n"
    "  -h, --help             print this help.\n"
    "\n"
    "All Rights Reserved.\n"
    "This program comes with absolutely no warranty.\n"
    "See the GNU General Public License, version 3 for details.\n"
    "\n", version, program.c_str(),
    defaults.default_input_file,
    defaults.default_output_file);
}

} // namespace
