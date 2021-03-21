
#include "catch.hpp"
#include "src/InputParser.h"
#include "src/packing.h"
#include "src/output.h"
#include <sstream>

namespace {
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
    parser.parse(input);
    static auto s_sprites = std::vector<Sprite>();
    s_sprites = std::move(parser).sprites();
    return pack_sprites(s_sprites);
  }

  [[maybe_unused]] void dump(PackedTexture texture) {
    static auto i = 0;
    texture.filename = FilenameSequence("dump-{000-}.png").get_nth_filename(i++);
    output_texture({ .debug = true }, texture);
  }

  PackedTexture pack_single_sheet(const char* definition) {
    auto textures = std::vector<PackedTexture>();
    REQUIRE_NOTHROW(textures = pack(definition));
    CHECK(textures.size() == 1);
    if (textures.size() != 1)
      return { };
  #if 0
    dump(textures[0]);
  #endif
    return textures[0];
  };
} // namespace

TEST_CASE("packing - Basic") {
  auto texture = pack_single_sheet(R"(
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 59, 58));

  texture = pack_single_sheet(R"(
    allow-rotate true
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 58, 58));

  texture = pack_single_sheet(R"(
    deduplicate true
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 55, 58));

  texture = pack_single_sheet(R"(
    allow-rotate true
    deduplicate true
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 55, 58));

  texture = pack_single_sheet(R"(
    max-width 128
    max-height 128
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width <= 128);
  CHECK(texture.height <= 128);
  CHECK(le_size(texture, 59, 58));

  texture = pack_single_sheet(R"(
    width 128
    max-height 128
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width == 128);
  CHECK(texture.height <= 128);
  CHECK(le_size(texture, 128, 32));

  texture = pack_single_sheet(R"(
    max-width 128
    height 128
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width <= 128);
  CHECK(texture.height == 128);
  CHECK(le_size(texture, 30, 128));

  texture = pack_single_sheet(R"(
    max-width 40
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width <= 40);
  CHECK(le_size(texture, 40, 85));

  texture = pack_single_sheet(R"(
    max-height 40
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 40);
  CHECK(le_size(texture, 89, 38));

  texture = pack_single_sheet(R"(
    power-of-two true
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width == 64);
  CHECK(texture.height == 64);

  texture = pack_single_sheet(R"(
    padding 1
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 63, 65));

  texture = pack_single_sheet(R"(
    padding 1
    power-of-two true
    input "test/Items.png"
      colorkey
  )");
  CHECK(ceil_to_pot(texture.width) == texture.width);
  CHECK(ceil_to_pot(texture.height) == texture.height);
  CHECK(le_size(texture, 64, 128));

  texture = pack_single_sheet(R"(
    max-height 16
    common-divisor 16
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 16);
  CHECK(le_size(texture, 496, 16));

  texture = pack_single_sheet(R"(
    padding 0 1
    common-divisor 16
    max-height 20
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 20);
  CHECK(le_size(texture, 498, 18));

  texture = pack_single_sheet(R"(
    padding 1
    common-divisor 16
    max-height 30
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 30);
  CHECK(le_size(texture, 528, 18));

  texture = pack_single_sheet(R"(
    padding 1 0
    common-divisor 16
    max-height 20
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 20);
  CHECK(le_size(texture, 526, 16));

  texture = pack_single_sheet(R"(
    max-height 30
    common-divisor 24
    extrude 1
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height <= 30);
  CHECK(le_size(texture, 806, 26));

  texture = pack_single_sheet(R"(
    padding 8 0
    common-divisor 16
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 136, 136));

  texture = pack_single_sheet(R"(
    padding 8 0
    deduplicate
    allow-rotate
    common-divisor 16
    input "test/Items.png"
      colorkey
  )");
  CHECK(le_size(texture, 112, 112));

  texture = pack_single_sheet(R"(
    padding 8 0
    max-width 16
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.width == 16);
  CHECK(le_size(texture, 492, 16));

  texture = pack_single_sheet(R"(
    padding 8 1
    max-height 18
    input "test/Items.png"
      colorkey
  )");
  CHECK(texture.height == 18);
  CHECK(le_size(texture, 478, 18));
}

TEST_CASE("packing - Errors") {
  CHECK_THROWS(pack(R"(
    padding 1
    max-width 16
    max-height 16
    input "test/Items.png"
      colorkey
  )"));

  CHECK_THROWS(pack(R"(
    padding 0 1
    max-width 16
    max-height 16
    input "test/Items.png"
      colorkey
  )"));
}

TEST_CASE("packing - Multiple sheets") {
  auto textures = pack(R"(
    allow-rotate
    deduplicate
    max-width 30
    square
    power-of-two
    input "test/Items.png"
      colorkey
  )");
  CHECK(textures.size() == 13);
  CHECK(textures[0].width <= 30);
  CHECK(textures[0].width == textures[0].height);
  CHECK(ceil_to_pot(textures[12].width) == textures[12].width);
  CHECK(textures[12].width == textures[12].height);

  CHECK_NOTHROW(textures = pack(R"(
    max-width 40
    max-height 40
    input "test/Items.png"
      colorkey
  )"));
  CHECK(textures.size() == 3);
  CHECK(le_size(textures[0], 39, 40));
  CHECK(le_size(textures[1], 38, 40));
  CHECK(le_size(textures[2], 16, 30));

  CHECK_NOTHROW(textures = pack(R"(
    max-width 40
    max-height 40
    square
    input "test/Items.png"
      colorkey
  )"));
  CHECK(textures.size() == 3);
  CHECK(le_size(textures[0], 40, 40));
  CHECK(le_size(textures[1], 32, 32));
  CHECK(textures[2].width == textures[2].height);

  CHECK_NOTHROW(textures = pack(R"(
    max-width 40
    max-height 40
    power-of-two true
    input "test/Items.png"
      colorkey
  )"));
  CHECK(textures.size() == 4);
  CHECK(le_size(textures[0], 32, 32));
  CHECK(le_size(textures[1], 32, 32));
  CHECK(le_size(textures[2], 32, 32));
  CHECK(le_size(textures[3], 32, 16));

  textures = pack("");
  CHECK(textures.size() == 0);

  textures = pack("padding 1");
  CHECK(textures.size() == 0);

  CHECK_NOTHROW(textures = pack(R"(
    max-width 16
    max-height 16
    input "test/Items.png"
      colorkey
  )"));
  CHECK(textures.size() == 14);
  CHECK(textures[0].width <= 16);
  CHECK(textures[0].height <= 16);

  CHECK_NOTHROW(textures = pack(R"(
    padding 1 0
    max-width 16
    max-height 16
    input "test/Items.png"
      colorkey
  )"));
  CHECK(textures.size() == 15);
  CHECK(textures[0].width <= 16);
  CHECK(textures[0].height <= 16);
}
