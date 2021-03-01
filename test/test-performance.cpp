
#include "catch.hpp"
#include "src/pack.h"
#include "src/image.h"
#include "src/FilenameSequence.h"
#include <random>

namespace {
  int random(int min, int max) {
    static auto s_rand = std::minstd_rand0();
    if (max <= min)
      return max;
    return min + s_rand() % (max - min + 1);
  }

  RGBA random_color() {
    auto c = []() { return static_cast<uint8_t>(random(0, 255)); };
    return RGBA{ { c(), c(), c(), 255u } };
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

  Image generate_image(const PackSheet& sheet, const std::vector<PackSize>& sizes) {
    auto image = Image(sheet.width, sheet.height);
    for (const auto& rect : sheet.rects) {
      auto w = sizes[rect.id].width;
      auto h = sizes[rect.id].height;
      if (rect.rotated)
        std::swap(w, h);
      fill_rect(image, { rect.x, rect.y, w, h }, random_color());
    }
    return image;
  }

  void dump(const Image& image) {
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
  CHECK(le_size(sheets[0], 56, 52));
}
