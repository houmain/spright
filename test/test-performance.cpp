
#include "catch.hpp"
#include "src/pack.h"
#include <random>

int random(int min, int max) {
  static auto s_rand = std::minstd_rand0();
  if (max <= min)
    return max;
  return min + s_rand() % (max - min + 1);
}

struct GeneratePackSizes {
  int count;
  int min_width;
  int max_width;
  int min_height;
  int max_height;
  bool rotate;
};

std::vector<PackSize> generate_pack_sizes(std::initializer_list<GeneratePackSizes> types) {
  auto id = 0;
  auto sizes = std::vector<PackSize>();
  for (const auto& type : types) {
    for (auto i = 0; i < type.count; ++i) {
      auto w = random(type.min_width, type.max_width);
      auto h = random(type.min_height, type.max_height);
      if (type.rotate && random(0, 1))
        std::swap(w, h);
      sizes.push_back({ id++, w, h });
    }
  }
  return sizes;
}

template<typename T>
bool le_size(const T& texture, int w, int h) {
  // here one can set a breakpoint to tighten the size constraints
  if (texture.width * texture.height < w * h)
    return true;

  if (texture.width * texture.height > w * h)
    return false;

  return true;
}

TEST_CASE("100 1x1", "[performance]") {
  auto sizes = generate_pack_sizes({
    { 100, 1, 1, 1, 1, true }
  });
  auto sheets = pack({ }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 10, 10));
}

TEST_CASE("Some", "[performance]") {
  auto sizes = generate_pack_sizes({
    { 10, 5, 5, 5, 9, true },
    { 10, 13, 13, 5, 9, true },
    { 10, 23, 23, 5, 9, true },
  });
  auto sheets = pack({ }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 55, 52));
}
