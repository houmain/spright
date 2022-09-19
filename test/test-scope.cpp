
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

using namespace spright;

TEST_CASE("scope - Tags") {
  auto input = std::stringstream(R"(
    input "test/Items.png"
      grid 16 16
      trim none
      tag "A"
        tag "B"
        sprite has_A_B# comment
        trim convex
          tag "C" # comment
          sprite has_A_B_C
            trim rect
        sprite has_A_B
      sprite has_A_D_E
        tag "D"
          trim rect
            tag "F"
          tag "E"
      tag "G"
        sprite has_A_G
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 5u);

  CHECK(sprites[0].id == "has_A_B");
  CHECK(sprites[0].tags.size() == 2u);
  CHECK(sprites[0].trim == Trim::none);

  CHECK(sprites[1].id == "has_A_B_C");
  CHECK(sprites[1].tags.size() == 3u);
  CHECK(sprites[1].trim == Trim::rect);

  CHECK(sprites[2].id == "has_A_B");
  CHECK(sprites[2].tags.size() == 2u);
  CHECK(sprites[2].tags.count("B") == 1u);
  CHECK(sprites[2].trim == Trim::convex);

  CHECK(sprites[3].id == "has_A_D_E");
  CHECK(sprites[3].tags.size() == 3u);
  CHECK(sprites[3].tags.count("B") == 0u);
  CHECK(sprites[3].tags.count("E") == 1u);
  CHECK(sprites[3].trim == Trim::rect);

  CHECK(sprites[4].id == "has_A_G");
  CHECK(sprites[4].tags.size() == 2u);
  CHECK(sprites[4].tags.count("B") == 0u);
  CHECK(sprites[4].tags.count("G") == 1u);
  CHECK(sprites[4].trim == Trim::none);
}

TEST_CASE("scope - Texture") {
  auto input = std::stringstream(R"(
    width 256
    output "tex1"
      padding 1
    output "tex2"
      padding 2
    width 128
    output "tex3"
      padding 3
    width 64
    input "test/Items.png"
      grid 16 16
      sprite
      sprite
        output "tex1"
      sprite
        output "tex2"
      sprite
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 4);
  CHECK(sprites[0].output->border_padding == 3);
  CHECK(sprites[1].output->border_padding == 1);
  CHECK(sprites[2].output->border_padding == 2);
  CHECK(sprites[0].output == sprites[3].output);
  CHECK(sprites[0].output->width == 128);
  CHECK(sprites[1].output->width == 256);
  CHECK(sprites[2].output->width == 256);
}
