
#include "settings.h"
#include "common.h"
#include <algorithm>
#include <sstream>

bool interpret_commandline(Settings& settings, int argc, const char* argv[]) {
  for (auto i = 1; i < argc; i++) {
    const auto argument = std::string_view(argv[i]);
    if (argument == "-i" || argument == "--input") {
      if (++i >= argc)
        return false;
      settings.input_files.push_back(std::filesystem::u8path(unquote(argv[i])));
    }
    else if (argument == "--") {
      auto ss = std::stringstream();
      std::copy(&argv[i + 1], &argv[i + argc],
        std::ostream_iterator<const char*>(ss, " "));
      settings.input = ss.str();
      std::replace(begin(settings.input), end(settings.input), ',', '\n');
      break;
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

  if (settings.input_files.empty() && settings.input.empty())
    settings.input_files.emplace_back(settings.default_input_file);

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
    "spright %s(c) 2020-2021 by Albert Kalchmair\n"
    "\n"
    "Usage: %s [-options]\n"
    "  -i, --input <file>     input definition file (default: %s).\n"
    "  -o, --output <file>    output description file (default: %s).\n"
    "  -t, --template <file>  description template file.\n"
    "  -a, --autocomplete     autocomplete input sheet description.\n"
    "  -d, --debug            draw sprite boundaries and pivot points on output.\n"
    "  -h, --help             print this help.\n"
    "  -- <args>              interpret remaining arguments as comma separated\n"
    "                         list of input definitions. WARNING: indentation after\n"
    "                         commas affects definition scope.\n"
    "\n"
    "All Rights Reserved.\n"
    "This program comes with absolutely no warranty.\n"
    "See the GNU General Public License, version 3 for details.\n"
    "\n", version, program.c_str(),
    defaults.default_input_file,
    defaults.default_output_file);
}
