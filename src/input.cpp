
#include "input.h"
#include "InputParser.h"
#include <fstream>

std::vector<Sprite> parse_definition(const Settings& settings) {
  auto input = std::fstream(settings.input_file, std::ios::in);
  if (!input.good())
    throw std::runtime_error("opening file '" + path_to_utf8(settings.input_file) + "' failed");

  auto parser = InputParser(settings);
  parser.parse(input);

  if (settings.autocomplete) {
    const auto output = parser.autocomplete_output();
    if (static_cast<int>(output.size()) != input.tellg()) {
      input.close();
      input = std::fstream(settings.input_file, std::ios::out);
      if (!input.good())
        throw std::runtime_error("opening file '" + path_to_utf8(settings.input_file) + "' failed");
      input << output;
    }
  }
  return std::move(parser).sprites();
}
