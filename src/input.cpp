
#include "input.h"
#include "InputParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <variant>

namespace spright {

InputDefinition parse_definition(const Settings& settings) {
  auto parser = InputParser(settings);

  if (settings.input_file.string() == "stdin") {
    parser.parse(std::cin);
  }
  else {
    auto input = std::fstream(settings.input_file, std::ios::in | std::ios::binary);
    if (!input.good())
      throw std::runtime_error("opening file '" + path_to_utf8(settings.input_file) + "' failed");
    parser.parse(input, settings.input_file);
  }

  if (settings.mode == Mode::autocomplete) {
    if (settings.output_file.string() == "stdout")
      std::cout << parser.autocomplete_output();
    else 
      update_textfile(settings.output_file, parser.autocomplete_output());
  }

  return {
    std::move(parser).inputs(),
    std::move(parser).sprites(),
    std::move(parser).descriptions(),
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
