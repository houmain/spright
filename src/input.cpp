
#include "input.h"
#include "InputParser.h"

std::vector<Sprite> parse_definition(const Settings& settings) {
  auto parser = InputParser(settings);
  parser.parse();
  return std::move(parser).sprites();
}
