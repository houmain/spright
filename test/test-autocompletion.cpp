
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

TEST_CASE("Grid", "[autocompletion]") {
  auto input = std::stringstream(R"(
sheet "test/Items.png"
  grid 16 16
  )");
  auto parser = InputParser(Settings{ .autocomplete = true });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  CHECK(sprites.size() == 18);

  auto text = parser.autocomplete_output();
  auto t= text.c_str();
  CHECK(text == R"(
sheet "test/Items.png"
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

TEST_CASE("Unaligned", "[autocompletion]") {
  auto input = std::stringstream(R"(
    sheet "test/Items.png"
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  CHECK(sprites.size() == 31);
}
