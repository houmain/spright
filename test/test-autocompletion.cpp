
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

TEST_CASE("autocompletion - Grid") {
  auto input = std::stringstream(R"(
input "test/Items.png"
  colorkey
  grid 16 16
  )");
  auto parser = InputParser(Settings{ .autocomplete = true });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 18);

  const auto text = parser.autocomplete_output();
  CHECK(text == R"(
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
    input "test/Items.png"
      colorkey
      atlas
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 31);
}

TEST_CASE("autocompletion - ID generator") {
  auto input = std::stringstream(R"(
    path "test"
    input "Items.png"
      colorkey
      atlas
      id "item_%i"
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 31);
  CHECK(sprites[10].id == "item_10");
}
