
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

using namespace spright;

TEST_CASE("pivot") {
  auto input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      grid 16 16
      sprite xword_yword
        pivot left top
      sprite xnum_yword
        pivot 1 top
      sprite xword_ynum
        pivot left 2
      sprite xnum_ynum
        pivot 1 2
      sprite yword_xword
        pivot top left
      sprite yword_xnum
        pivot top 1
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 6u);

  CHECK(sprites[0].id == "xword_yword");
  CHECK(sprites[0].pivot.x == PivotX::left);
  CHECK(sprites[0].pivot.y == PivotY::top);

  CHECK(sprites[1].id == "xnum_yword");
  CHECK(sprites[1].pivot.x == PivotX::left);
  CHECK(sprites[1].pivot_point.x == 1);
  CHECK(sprites[1].pivot.y == PivotY::top);

  CHECK(sprites[2].id == "xword_ynum");
  CHECK(sprites[2].pivot.x == PivotX::left);
  CHECK(sprites[2].pivot.y == PivotY::top);
  CHECK(sprites[2].pivot_point.y == 2);

  CHECK(sprites[3].id == "xnum_ynum");
  CHECK(sprites[3].pivot.x == PivotX::left);
  CHECK(sprites[3].pivot_point.x == 1);
  CHECK(sprites[3].pivot.y == PivotY::top);
  CHECK(sprites[3].pivot_point.y == 2);

  CHECK(sprites[4].id == "yword_xword");
  CHECK(sprites[4].pivot.x == PivotX::left);
  CHECK(sprites[4].pivot.y == PivotY::top);

  CHECK(sprites[5].id == "yword_xnum");
  CHECK(sprites[5].pivot.x == PivotX::left);
  CHECK(sprites[5].pivot_point.x == 1);
  CHECK(sprites[5].pivot.y == PivotY::top);

  // not allowed combination (use: top + 2 left)
  input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      grid 16 16
      sprite ynum_xword
        pivot 2 left
  )");
  CHECK_THROWS(parser.parse(input));

  // duplicate coordinates
  input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      grid 16 16
      sprite yword_yword
        pivot middle top
  )");
  CHECK_THROWS(parser.parse(input));
}

TEST_CASE("pivot - expressions") {
  auto input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      grid 16 16
      sprite expr1
        pivot left - 4 middle
      sprite expr2
        pivot -3 bottom + 4
      sprite expr3
        pivot 3 -7
      sprite expr4
        pivot middle - 4.2 + 2.5 center + 3.5 - 1.1
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 4u);

  CHECK(sprites[0].id == "expr1");
  CHECK(sprites[0].pivot.x == PivotX::left);
  CHECK(sprites[0].pivot_point.x == -4);
  CHECK(sprites[0].pivot.y == PivotY::middle);
  CHECK(sprites[0].pivot_point.y == 0);

  CHECK(sprites[1].id == "expr2");
  CHECK(sprites[1].pivot.x == PivotX::left);
  CHECK(sprites[1].pivot_point.x == -3);
  CHECK(sprites[1].pivot.y == PivotY::bottom);
  CHECK(sprites[1].pivot_point.y == 4);

  CHECK(sprites[2].id == "expr3");
  CHECK(sprites[2].pivot.x == PivotX::left);
  CHECK(sprites[2].pivot_point.x == 3);
  CHECK(sprites[2].pivot.y == PivotY::top);
  CHECK(sprites[2].pivot_point.y == -7);

  CHECK(sprites[3].id == "expr4");
  CHECK(sprites[3].pivot.x == PivotX::center);
  CHECK(sprites[3].pivot_point.x == 3.5 - 1.1);
  CHECK(sprites[3].pivot.y == PivotY::middle);
  CHECK(sprites[3].pivot_point.y == -4.2 + 2.5);

  // not allowed combination (use: middle left - 3)
  input = std::stringstream(R"(
    sheet "sprites"
    input "test/Items.png"
      grid 16 16
      sprite
        pivot middle -3
  )");
  CHECK_THROWS(parser.parse(input));
}
