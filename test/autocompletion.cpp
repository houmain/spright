
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

TEST_CASE("Grid", "[autocompletion]") {
  auto input = std::stringstream(R"(
    sheet "test/Items.png"
      grid 16 16
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  CHECK(sprites.size() == 18);
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
