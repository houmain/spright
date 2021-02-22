
#include "catch.hpp"
#include "src/InputParser.h"
#include "src/packing.h"
#include <sstream>

template<typename T>
bool le_size(const T& texture, int w, int h) {
  // here one can set a breakpoint to tighten the size constraints
  if (texture.width * texture.height < w * h)
    return true;

  if (texture.width * texture.height > w * h)
    return false;

  return true;
}

std::vector<PackedTexture> pack(const char* definition) {
  auto input = std::stringstream(definition);
  auto parser = InputParser(Settings{ });
  REQUIRE_NOTHROW(parser.parse(input));
  auto sprites = std::move(parser).sprites();
  return pack_sprites(sprites);
}

PackedTexture pack_single_sheet(const char* definition) {
  auto textures = ::pack(definition);
  CHECK(textures.size() == 1);
  return (textures.empty() ? PackedTexture() : textures[0]);
};

TEST_CASE("Basic", "[packing]") {
  auto texture = pack_single_sheet(R"(
    sheet "test/Items.png"
  )");
  CHECK(le_size(texture, 58, 59));

  texture = pack_single_sheet(R"(
    allow-rotate true
    sheet "test/Items.png"
  )");
  CHECK(le_size(texture, 58, 59));

  texture = pack_single_sheet(R"(
    deduplicate true
    sheet "test/Items.png"
  )");
  CHECK(le_size(texture, 55, 58));

  texture = pack_single_sheet(R"(
    allow-rotate true
    deduplicate true
    sheet "test/Items.png"
  )");
  CHECK(le_size(texture, 58, 55));

  texture = pack_single_sheet(R"(
    max-width 128
    max-height 128
    sheet "test/Items.png"
  )");
  CHECK(texture.width <= 128);
  CHECK(texture.height <= 128);
  CHECK(le_size(texture, 58, 59));

  texture = pack_single_sheet(R"(
    width 128
    max-height 128
    sheet "test/Items.png"
  )");
  CHECK(texture.width == 128);
  CHECK(texture.height <= 128);
  CHECK(le_size(texture, 128, 32));

  texture = pack_single_sheet(R"(
    max-width 128
    height 128
    sheet "test/Items.png"
  )");
  CHECK(texture.width <= 128);
  CHECK(texture.height == 128);
  CHECK(le_size(texture, 30, 128));

  texture = pack_single_sheet(R"(
    max-width 40
    sheet "test/Items.png"
  )");
  CHECK(texture.width <= 40);
  CHECK(le_size(texture, 40, 85));

  texture = pack_single_sheet(R"(
    max-height 40
    sheet "test/Items.png"
  )");
  CHECK(texture.height <= 40);
  CHECK(le_size(texture, 86, 40));

  texture = pack_single_sheet(R"(
    power-of-two true
    sheet "test/Items.png"
  )");
  CHECK(texture.width == 64);
  CHECK(texture.height == 64);

  texture = pack_single_sheet(R"(
    padding 1
    sheet "test/Items.png"
  )");
  CHECK(le_size(texture, 64, 66));

  texture = pack_single_sheet(R"(
    padding 1
    power-of-two true
    sheet "test/Items.png"
  )");
  CHECK(ceil_to_pot(texture.width) == texture.width);
  CHECK(ceil_to_pot(texture.height) == texture.height);
  CHECK(le_size(texture, 64, 128));
}
