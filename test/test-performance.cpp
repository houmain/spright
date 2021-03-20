
#include "catch.hpp"
#include "src/image.h"
#include "src/FilenameSequence.h"
#include "rect_pack/rect_pack.h"
#include <random>

namespace {
  struct GeneratePackSizes {
    int count;
    int min_width;
    int min_height;
    int max_width;
    int max_height;
    bool rotate;
  };

  std::vector<rect_pack::Size> generate_pack_sizes(std::initializer_list<GeneratePackSizes> types) {
    auto rand = std::minstd_rand0();
    const auto random = [&](int min, int max) -> int {
      if (max <= min)
        return max;
      return min + rand() % (max - min + 1);
    };
    auto id = 0;
    auto sizes = std::vector<rect_pack::Size>();
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

  [[maybe_unused]] Image generate_image(const rect_pack::Sheet& sheet,
      const std::vector<rect_pack::Size>& sizes) {

    const auto random_color = [](int seed) {
      const auto c = [&]() {
        seed += 423;
        seed *= 123;
        return static_cast<uint8_t>(seed);
      };
      return RGBA{ { c(), c(), c(), 255u } };
    };
    auto image = Image(sheet.width, sheet.height);
    for (const auto& rect : sheet.rects) {
      auto w = sizes[rect.id].width;
      auto h = sizes[rect.id].height;
      if (rect.rotated)
        std::swap(w, h);
      fill_rect(image, { rect.x, rect.y, w, h }, random_color(rect.id));
    }
    return image;
  }

  [[maybe_unused]] void dump(const Image& image) {
    static auto i = 0;
    auto filename = FilenameSequence("dump-{000-}.png").get_nth_filename(i++);
    save_image(image, filename);
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
} // namespace

TEST_CASE("performance - 100 pixels") {
  auto sizes = generate_pack_sizes({
    { 100, 1, 1, 1, 1, true }
  });
  auto sheets = pack({ .allow_rotate = true }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 10, 10));
}

TEST_CASE("performance - Some") {
  auto sizes = generate_pack_sizes({
    { 10, 5, 5, 5, 9, true },
    { 10, 13, 5, 13, 9, true },
    { 10, 23, 5, 23, 9, true },
  });
  auto sheets = pack({ .allow_rotate = true }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 57, 57));
}

TEST_CASE("performance - Big") {
  auto sizes = generate_pack_sizes({
    { 10, 30, 30, 400, 400, true }
  });
  auto sheets = pack({ .allow_rotate = true }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 533, 570));
}

TEST_CASE("performance - Long") {
  auto sizes = generate_pack_sizes({
    { 50, 30, 30, 50, 400, true },
  });
  auto sheets = pack({ .allow_rotate = true }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 660, 632));
}

TEST_CASE("performance - 1000 Skyline") {
  auto sizes = generate_pack_sizes({
    { 333, 5, 5, 15, 10, true },
    { 333, 13, 10, 23, 20, true },
    { 333, 23, 20, 33, 30, true },
  });
  auto sheets = pack({
    .method = rect_pack::Method::Best_Skyline,
    .allow_rotate = true
  }, sizes);
  REQUIRE(sheets.size() == 1);
  CHECK(le_size(sheets[0], 534, 653));

  //dump(generate_image(sheets[0], sizes));
}
