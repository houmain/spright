
#include "settings.h"
#include "common.h"

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
    else if (argument == "-s" || argument == "--sheet") {
      if (++i >= argc)
        return false;
      settings.sheet_file = std::filesystem::u8path(unquote(argv[i]));
    }
    else if (argument == "-a" || argument == "--autocomplete") {
      settings.autocomplete = true;
    }
    else if (argument == "-d" || argument == "--debug") {
      settings.debug = true;
    }
    else {
      return false;
    }
  }
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
    "spright %s(c) 2020 by Albert Kalchmair\n"
    "\n"
    "Usage: %s [-options]\n"
    "  -i, --input <file>     input sheet description (required).\n"
    "  -o, --output <file>    output sheet description (default: %s).\n"
    "  -s, --sheet <file>     output sheet image (default: %s).\n"
    "  -t, --template <file>  output sheet description template.\n"
    "  -a, --autocomplete     autocomplete input sheet description.\n"
    "  -d, --debug            draw rectangles and pivot points on sprites.\n"
    "  -h, --help             print this help.\n"
    "\n"
    "All Rights Reserved.\n"
    "This program comes with absolutely no warranty.\n"
    "See the GNU General Public License, version 3 for details.\n"
    "\n", version, program.c_str(),
    defaults.output_file.u8string().c_str(),
    defaults.sheet_file.u8string().c_str());
}
