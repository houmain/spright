
#include "catch.hpp"
#include "src/InputParser.h"
#include "src/trimming.h"
#include "src/packing.h"
#include "src/output.h"

using namespace spright;

namespace {
  std::pair<std::vector<Sprite>, std::vector<Slice>> pack(const char* definition) {
    const auto settings = Settings{ };
    auto input = std::stringstream(definition);
    auto parser = InputParser(settings);
    parser.parse(input);
    auto sprites = std::move(parser).sprites();
    for (auto& sprite : sprites)
      trim_sprite(sprite);
    auto slices = pack_sprites(sprites);
    auto variables = VariantMap{ };
    evaluate_expressions(settings, sprites, slices, variables);
    return { std::move(sprites), std::move(slices) };
  }
} // namespace

TEST_CASE("templates - Empty") {
  const auto [sprites, slices] = pack(R"(
    input "test/Items.png"
  )");
  const auto description = get_description(
R"(
)", sprites, slices);
  CHECK(description ==
R"(
)");
}

TEST_CASE("templates - getIdOrFilename") {
  const auto [sprites, slices] = pack(R"(
    id "{{ source.filename }}"
    input "test/Items.png"
  )");

  const auto description = get_description(R"(
let sprite_ids = [{% for sprite in sprites %}"{{ sprite.id }}",{% endfor %}];
)", sprites, slices);

  CHECK(description == R"(
let sprite_ids = ["test/Items.png",];
)");
}

TEST_CASE("templates - removeExtension") {
  const auto [sprites, slices] = pack(R"(
    path "test"
    id "{{ source.filename }}"
    input "Items.png"
  )");

  const auto description = get_description(R"(
let sprite_ids = [{% for sprite in sprites %}"{{ removeExtension(sprite.id) }}",{% endfor %}];
)", sprites, slices);

  CHECK(description == R"(
let sprite_ids = ["Items",];
)");
}
