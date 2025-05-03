
#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "gifenc/gifenc.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  using RGBASpan = span<RGBA>;

  // https://en.wikipedia.org/wiki/Median_cut
  std::vector<RGBA> median_cut_reduction(RGBASpan image, int max_colors) {
    struct Bucket {
      RGBA::Channel max_channel_range;
      RGBASpan colors;
    };
  
    auto buckets = std::vector<Bucket>();
    const auto insert_bucket = [&](RGBASpan colors) {
      // compute channel with maximum range
      auto max_channel = 0;
      auto max_channel_range = RGBA::Channel{ };
      for (auto i = 0; i < 4; ++i) {
        const auto [min, max] = std::minmax_element(colors.begin(), colors.end(),
          [&](const RGBA& a, const RGBA& b) { return a.channel(i) < b.channel(i); });
        const auto channel_range = RGBA::to_channel(max->channel(i) - min->channel(i));
        if (channel_range > max_channel_range) {
          max_channel_range = channel_range;
          max_channel = i;
        }
      }

      // sort colors by this channel
      std::sort(colors.begin(), colors.end(), 
        [&](const RGBA& a, const RGBA& b) { 
          return a.channel(max_channel) < b.channel(max_channel); 
        });

      // insert sorted in bucket list
      auto bucket = Bucket{ max_channel_range, colors };
      buckets.insert(std::lower_bound(buckets.begin(), buckets.end(), bucket,
        [](const Bucket& a, const Bucket& b) {
          return (a.max_channel_range < b.max_channel_range);
        }), bucket);
    };

    // start with one bucket containing whole image
    insert_bucket(image);

    while (to_int(buckets.size()) < max_colors) {
      // split bucket with maximum range
      auto [range, colors] = buckets.back();
      if (range == 0)
        break;

      buckets.pop_back();
      insert_bucket(colors.subspan(0, colors.size() / 2));
      insert_bucket(colors.subspan(colors.size() / 2));
    }

    // get average colors of buckets
    auto palette = Palette();
    for (const auto& bucket : buckets) {
      auto sum = std::array<uint32_t, 4>();
      for (const auto& color : bucket.colors)
        for (auto i = 0; i < 4; ++i)
          sum[to_unsigned(i)] += color.channel(i);
      auto color = RGBA{ };
      for (auto i = 0; i < 4; ++i)
        color.channel(i) = RGBA::to_channel(sum[to_unsigned(i)] / bucket.colors.size());
      palette.push_back(color);
    }
    return palette;
  }

  int index_of_closest_palette_color(const Palette& palette, const RGBA& color) {
    auto min_index = 0;
    auto min_distance = std::numeric_limits<int>::max();
    for (auto i = 0u; i < palette.size(); ++i) {
      const auto r = palette[i].r - color.r;
      const auto g = palette[i].g - color.g;
      const auto b = palette[i].b - color.b;
      const auto distance = (r * r + g * g + b * b);
      if (distance < min_distance) {
        min_index = to_int(i);
        min_distance = distance;
      }
    }
    return min_index;
  }

  const RGBA& closest_palette_color(const Palette& palette, const RGBA& color) {
    return palette[to_unsigned(index_of_closest_palette_color(palette, color))];
  }

  // https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
  void floyd_steinberg_dithering(ImageView<RGBA> image_rgba, const Palette& palette) {
    const auto diff = [](const RGBA::Channel& a, const RGBA::Channel& b) { 
      return static_cast<int>(a) - static_cast<int>(b);
    };
    const auto saturate = [](int value) { 
      return RGBA::to_channel(std::clamp(value, 0, 255));
    };
    const auto w = image_rgba.width();
    const auto h = image_rgba.height();
    for (auto y = 0; y < h; ++y)
      for (auto x = 0; x < w; ++x) {
        auto& color = image_rgba.value_at({ x, y });
        const auto old_color = color;
        color = closest_palette_color(palette, color);
        const auto error_r = diff(old_color.r, color.r);
        const auto error_g = diff(old_color.g, color.g);
        const auto error_b = diff(old_color.b, color.b);
        const auto apply_error = [&](int x, int y, int fs) {
          auto& color = image_rgba.value_at({ 
            std::clamp(x, 0, w - 1), std::clamp(y, 0, h - 1)
          });
          color.r = saturate(color.r + error_r * fs / 16);
          color.g = saturate(color.g + error_g * fs / 16);
          color.b = saturate(color.b + error_b * fs / 16);
        };
        apply_error(x + 1, y,     7);
        apply_error(x - 1, y + 1, 3);
        apply_error(x    , y + 1, 5);
        apply_error(x + 1, y + 1, 1);
      }
  }

  [[maybe_unused]] Palette generate_palette(const Image& image, int max_colors) {
    auto clone = clone_image(image);
    const auto clone_rgba = clone.view<RGBA>();
    return median_cut_reduction(
      { clone_rgba.values(), to_unsigned(clone_rgba.size()) }, max_colors);
  }

  Palette generate_palette(const Animation& animation, int max_colors) {
    auto merged_size = size_t{ };
    for (const auto& frame : animation.frames)
      merged_size += frame.image.data().size_bytes();
    auto merged = std::make_unique<std::byte[]>(merged_size);
    auto pos = merged.get();
    for (const auto& frame : animation.frames) {
      std::memcpy(pos, frame.image.data().data(), frame.image.size_bytes());
      pos += frame.image.size_bytes();
    }
    return median_cut_reduction({
        reinterpret_cast<RGBA*>(merged.get()), 
        merged_size / sizeof(RGBA)
      }, max_colors);
  }

  Image quantize_image(ImageView<RGBA> image_rgba, const Palette& palette) {
    auto out = Image(ImageType::Mono, image_rgba.width(), image_rgba.height());
    const auto out_mono = out.view<RGBA::Channel>();
    for (auto y = 0; y < image_rgba.height(); ++y)
      for (auto x = 0; x < image_rgba.width(); ++x)
        out_mono.value_at({ x, y }) = RGBA::to_channel(
          index_of_closest_palette_color(palette, 
            image_rgba.value_at({ x, y })));
    return out;
  }

  // https://giflib.sourceforge.net/whatsinagif/
  bool write_gif(const std::string& filename, const Animation& animation) {
    if (animation.frames.empty())
      return false;

    const auto palette = generate_palette(animation, 
      (animation.max_colors ? std::min(animation.max_colors, 256) : 256));
    auto bits = 0;
    for (auto c = palette.size() - 1; c; c >>= 1)
      ++bits;
    auto palette_rgb = std::make_unique<RGBA::Channel[]>((1 << bits) * 3);
    auto pos = palette_rgb.get();
    for (const auto& color : palette) {
      *pos++ = color.r;
      *pos++ = color.g;
      *pos++ = color.b;
    }

    auto transparent_index = -1;
    if (animation.color_key)
      transparent_index = index_of_closest_palette_color(
        palette, *animation.color_key);

    auto width = 0;
    auto height = 0;
    for (const auto& frame : animation.frames) {
      width = std::max(width, frame.image.width());
      height = std::max(height, frame.image.height());
    }
    if (width > 0xFFFF || height > 0xFFFF)
      return false;

    auto gif = ge_new_gif(filename.c_str(),
      static_cast<uint16_t>(width),
      static_cast<uint16_t>(height),
      palette_rgb.get(), bits, transparent_index, animation.loop_count);
    if (!gif)
      return false;

    for (const auto& frame : animation.frames) {
      const auto delay = std::chrono::duration_cast<
        std::chrono::duration<uint16_t, std::ratio<1, 100>>>(
        std::chrono::duration<real>(frame.duration)).count();

      auto dithered = clone_image(frame.image);
      floyd_steinberg_dithering(dithered.view<RGBA>(), palette);
      const auto mono = quantize_image(dithered.view<RGBA>(), palette);
      std::memcpy(gif->frame, mono.data().data(), mono.size_bytes());
      ge_add_frame(gif, delay);
    }
    ge_close_gif(gif);
    return true;
  }
} // namespace

Image load_image(const std::filesystem::path& filename) {
  auto width = 0;
  auto height = 0;
  auto data = std::add_pointer_t<std::byte>{ };

#if defined(EMBED_TEST_FILES)
  if (filename == "test/Items.png") {
    unsigned char file[] {
#include "test/Items.png.inc"
    };
    auto channels = 0;
    data = reinterpret_cast<std::byte*>(stbi_load_from_memory(
        file, sizeof(file), &width, &height, &channels, sizeof(RGBA)));
  }
  else
#endif

#if defined(_WIN32)
  if (auto file = _wfopen(filename.wstring().c_str(), L"rb")) {
#else
  if (auto file = std::fopen(path_to_utf8(filename).c_str(), "rb")) {
#endif
    auto channels = 0;
    data = reinterpret_cast<std::byte*>(stbi_load_from_file(
        file, &width, &height, &channels, sizeof(RGBA)));
    std::fclose(file);
  }
  if (!data)
    throw std::runtime_error("loading file '" + 
      path_to_utf8(filename) + "' failed");

  return Image(ImageType::RGBA, width, height, data);
}

void save_image(const Image& image, const std::filesystem::path& path) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  const auto filename = path_to_utf8(path);

  const auto result = [&]() -> bool {
    const auto extension = to_lower(path_to_utf8(path.extension()));
    if (extension == ".gif") {
      auto animation = Animation{ };
      animation.frames.push_back({ 0, clone_image(image), 0.0 });
      return write_gif(filename, animation);
    }

    const auto comp = to_int(sizeof(RGBA));
    const auto image_rgba = image.view<RGBA>();
    if (extension == ".png")
      return stbi_write_png(filename.c_str(),
        image.width(), image.height(), comp, image_rgba.values(), 0);

    if (extension == ".bmp")
      return stbi_write_bmp(filename.c_str(),
        image.width(), image.height(), comp, image_rgba.values());

    stbi_write_tga_with_rle = 1;
    if (extension == ".tga")
      return stbi_write_tga(filename.c_str(), 
        image.width(), image.height(), comp, image_rgba.values());

    error("unsupported image file format '", filename, "'");
  }();
  if (!result)
    error("writing file '", filename, "' failed");
}

void save_animation(const Animation& animation, const std::filesystem::path& path) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  const auto filename = path_to_utf8(path);
  const auto extension = to_lower(path_to_utf8(path.extension()));
  if (!(extension == ".gif" && write_gif(filename, animation)))
    error("writing file '", filename, "' failed");
}

} // namespace
