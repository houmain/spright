
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

using namespace spright;

TEST_CASE("autocompletion - Grid") {
  auto input = std::stringstream(R"(
sheet "sprites"
input "test/Items.png"
  colorkey
  grid 16 16
  )");
  auto parser = InputParser(Settings{ .mode = Mode::autocomplete });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 18);

  const auto text = parser.autocomplete_output();
  CHECK(text == R"(
sheet "sprites"
input "test/Items.png"
  colorkey
  grid 16 16
  row 1
    skip
    sprite 
    sprite 
    sprite 
    sprite 
    sprite 
    skip
    sprite 
  row 3
    skip
    sprite 
    sprite 
    skip
    sprite 
    skip
    sprite 
    skip
    sprite 
  row 5
    skip
    sprite 
    sprite 
    skip
    sprite 
    skip
    sprite 
    sprite 
    sprite 
    sprite 
)");
}

TEST_CASE("autocompletion - Unaligned") {
  auto input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      colorkey
      atlas
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 31);
}
