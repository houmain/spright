
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

using namespace spright;

namespace {
  InputParser parse(const char* config) {
    auto parser = InputParser(Settings{ });
    auto input = std::stringstream(config);
    parser.parse(input);
    return parser;
  }
} // namespace

TEST_CASE("scope - Tags") {
  auto parser = parse(R"(
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
          tag "E"
      tag "G"
        sprite has_A_G
  )");
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

TEST_CASE("scope - Output/Sprite") {
  auto parser = parse(R"(
    width 256
    sheet "tex1"
      padding 1
    sheet "tex2"
      padding 2
    width 128
    sheet "tex3"
      padding 3
    input "test/Items.png"
      grid 16 16
      sprite
      sprite
        sheet "tex1"
      sprite
        sheet "tex2"
      sprite
  )");
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 4);
  CHECK(sprites[0].sheet->border_padding == 3);
  CHECK(sprites[1].sheet->border_padding == 1);
  CHECK(sprites[2].sheet->border_padding == 2);
  CHECK(sprites[0].sheet == sprites[3].sheet);
  CHECK(sprites[0].sheet->width == 128);
  CHECK(sprites[1].sheet->width == 256);
  CHECK(sprites[2].sheet->width == 256);
}

TEST_CASE("scope - Problems") {
  // sprite not on input
  CHECK_THROWS(parse(R"(
    sheet "tex1"
    input "test/Items.png"
    sprite "text"
  )"));

  // definition without effect
  CHECK_NOTHROW(parse(R"(
    sheet "tex1"
      width 100
    input "test/Items.png"
      sprite "text"
  )"));

  CHECK_NOTHROW(parse(R"(
    width 100
    sheet "tex1"
    input "test/Items.png"
      sprite "text"
  )"));

  CHECK_THROWS(parse(R"(
    sheet "tex1"
    width 100
    input "test/Items.png"
      sprite "text"
  )"));

  // row, skip, span without grid
  CHECK_THROWS(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      row 1
      sprite "text"
  )"));

  CHECK_THROWS(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      sprite "text"
      skip
  )"));

  CHECK_THROWS(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      sprite "text"
        span 2 2
  )"));

  // extra scopes are allowed
  CHECK_NOTHROW(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      tag anim "anim"
        sprite "text"
  )"));

  CHECK_NOTHROW(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      grid 16 16
      tag anim "anim"
        row 0
          sprite "text"
          skip
          sprite "text"

      tag anim "anim2"
        row 1
          skip
          sprite "text"
  )"));

  CHECK_NOTHROW(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      group
        tag anim "anim"
        sprite "text"
        sprite "text"
      sprite "text"
  )"));

  // definition in wrong scope
  CHECK_THROWS(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      width 100
      sprite "text"
  )"));

  // sheet without sprites is tolerated
  CHECK_NOTHROW(parse(R"(
    sheet "tex1"
    input "test/Items.png"
      sprite "text"
  )"));

  CHECK_NOTHROW(parse(R"(
    input "test/Items.png"
      sprite "text"
    sheet "tex1"
  )"));
}
