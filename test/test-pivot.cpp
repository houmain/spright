
#include "catch.hpp"
#include "src/InputParser.h"
#include <sstream>

using namespace spright;

TEST_CASE("pivot") {
  auto input = std::stringstream(R"(
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
      sprite ynum_xword
        pivot 2 left
  )");
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  const auto& sprites = parser.sprites();
  REQUIRE(sprites.size() == 7u);

  CHECK(sprites[0].id == "xword_yword");
  CHECK(sprites[0].pivot.x == PivotX::left);
  CHECK(sprites[0].pivot.y == PivotY::top);

  CHECK(sprites[1].id == "xnum_yword");
  CHECK(sprites[1].pivot.x == PivotX::custom);
  CHECK(sprites[1].pivot_point.x == 1);
  CHECK(sprites[1].pivot.y == PivotY::top);

  CHECK(sprites[2].id == "xword_ynum");
  CHECK(sprites[2].pivot.x == PivotX::left);
  CHECK(sprites[2].pivot.y == PivotY::custom);
  CHECK(sprites[2].pivot_point.y == 2);

  CHECK(sprites[3].id == "xnum_ynum");
  CHECK(sprites[3].pivot.x == PivotX::custom);
  CHECK(sprites[3].pivot_point.x == 1);
  CHECK(sprites[3].pivot.y == PivotY::custom);
  CHECK(sprites[3].pivot_point.y == 2);

  CHECK(sprites[4].id == "yword_xword");
  CHECK(sprites[4].pivot.x == PivotX::left);
  CHECK(sprites[4].pivot.y == PivotY::top);

  CHECK(sprites[5].id == "yword_xnum");
  CHECK(sprites[5].pivot.x == PivotX::custom);
  CHECK(sprites[5].pivot_point.x == 1);
  CHECK(sprites[5].pivot.y == PivotY::top);

  CHECK(sprites[6].id == "ynum_xword");
  CHECK(sprites[6].pivot.x == PivotX::left);
  CHECK(sprites[6].pivot.y == PivotY::custom);
  CHECK(sprites[6].pivot_point.y == 2);
}
