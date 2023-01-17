
#include "input.h"
#include "InputParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <variant>

namespace spright {

std::vector<Sprite> parse_definition(const Settings& settings) {
  auto parser = InputParser(settings);

  if (!settings.input.empty()) {
    auto input = std::stringstream(settings.input);
    parser.parse(input);
  }

  for (const auto& input_file : settings.input_files) {

    if (input_file == "stdin") {
      parser.parse(std::cin);
      continue;
    }

    auto input = std::fstream(input_file, std::ios::in | std::ios::binary);
    if (!input.good())
      throw std::runtime_error("opening file '" + path_to_utf8(input_file) + "' failed");

    parser.parse(input);

    if (settings.autocomplete) {
      const auto output = parser.autocomplete_output();
      if (static_cast<int>(output.size()) != input.tellg()) {
        input.close();
        input = std::fstream(input_file, std::ios::out);
        if (!input.good())
          throw std::runtime_error("writing file '" + path_to_utf8(input_file) + "' failed");
        input << output;
      }
    }
  }

  return std::move(parser).sprites();
}

} // namespace
