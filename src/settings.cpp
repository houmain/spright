
#include "settings.h"
#include "common.h"
#include <algorithm>
#include <sstream>

namespace {
  // replace comma with newline (not within string)
  // skip spaces after newline
  std::string arglist_to_input(std::string string) {
    auto pos = begin(string);
    auto in_string = false;
    auto after_newline = false;
    for (auto it = begin(string); it != end(string); ++it) {
      auto c = *it;
      if (c == '"') {
        in_string = !in_string;
      }
      else if (!in_string && c == ',') {
        c = '\n';
      }
      else if (after_newline && c == ' ') {
        continue;
      }
      *pos++ = c;
      after_newline = (c == '\n');
    }
    string.erase(pos, end(string));
    return string;
  }
} // namespace

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
      settings.input = arglist_to_input(ss.str());
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
    else if (argument == "-p" || argument == "--path") {
      if (++i >= argc)
        return false;
      settings.output_path = std::filesystem::u8path(unquote(argv[i]));
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
    "  -t, --template <file>  template for output description.\n"
    "  -p, --path <path>      path to prepend to all output files.\n"
    "  -a, --autocomplete     autocomplete input definition.\n"
    "  -d, --debug            draw sprite boundaries and pivot points on output.\n"
    "  -h, --help             print this help.\n"
    "  -- <args>              interpret remaining arguments as a comma separated\n"
    "                         list of input definitions.\n"
    "\n"
    "All Rights Reserved.\n"
    "This program comes with absolutely no warranty.\n"
    "See the GNU General Public License, version 3 for details.\n"
    "\n", version, program.c_str(),
    defaults.default_input_file,
    defaults.default_output_file);
}
