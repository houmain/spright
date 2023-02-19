
#include "input.h"
#include "InputParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <variant>

namespace spright {

InputDefinition parse_definition(const Settings& settings) {
  auto parser = InputParser(settings);

  for (const auto& input_file : settings.input_files) {
    if (input_file == "stdin") {
      parser.parse(std::cin);
      continue;
    }

    auto input = std::fstream(input_file, std::ios::in | std::ios::binary);
    if (!input.good())
      throw std::runtime_error("opening file '" + path_to_utf8(input_file) + "' failed");
    parser.parse(input, input_file);
    input.close();

    if (settings.autocomplete)
      update_textfile(input_file, parser.autocomplete_output());
  }

  return {
    std::move(parser).sprites(),
    std::move(parser).variables()
  };
}

int get_max_slice_count(const Sheet& sheet) {
  auto max_count = std::numeric_limits<int>::max();
  for (const auto& output : sheet.outputs)
    max_count = std::min(max_count, output->filename.count());
  return max_count;
}

} // namespace
