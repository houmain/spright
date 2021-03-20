
#include "catch.hpp"
#include "src/InputParser.h"
#include "src/packing.h"
#include "src/output.h"

namespace {
  std::pair<std::vector<Sprite>, std::vector<PackedTexture>> pack(const char* definition) {
    auto input = std::stringstream(definition);
    auto parser = InputParser(Settings{ });
    parser.parse(input);
    auto sprites = std::move(parser).sprites();
    auto textures = pack_sprites(sprites);
    return { std::move(sprites), std::move(textures) };
  }
} // namespace

TEST_CASE("templates - Empty") {
  const auto [sprites, textures] = pack(R"(
    input "test/Items.png"
  )");
  const auto description = get_description(
R"(
)", sprites, textures);
  CHECK(description ==
R"(
)");
}

TEST_CASE("templates - getIdOrFilename") {
  const auto [sprites, textures] = pack(R"(
    input "test/Items.png"
  )");

  const auto description = get_description(R"(
let sprite_ids = [{% for sprite in sprites %}"{{ getIdOrFilename(sprite) }}",{% endfor %}];
)", sprites, textures);

  CHECK(description == R"(
let sprite_ids = ["test/Items.png",];
)");
}

TEST_CASE("templates - removeExtension") {
  const auto [sprites, textures] = pack(R"(
    path "test"
    input "Items.png"
  )");

  const auto description = get_description(R"(
let sprite_ids = [{% for sprite in sprites %}"{{ removeExtension(getIdOrFilename(sprite)) }}",{% endfor %}];
)", sprites, textures);

  CHECK(description == R"(
let sprite_ids = ["Items",];
)");
}
