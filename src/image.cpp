
#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image_resize.h"
#include "texpack/bleeding.h"
#include "gifenc/gifenc.h"
#include "nonstd/span.hpp"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  using RGBASpan = nonstd::span<RGBA>;

  inline void check(bool inside) {
    if (!inside)
      throw std::logic_error("access outside image bounds");
  }

  inline void check_rect(const Image& image, const Rect& rect) {
    check(containing(image.bounds(), rect));
  }

  inline const RGBA& checked_rgba_at(const Image& image, const Point& p) {
    check(containing(image.bounds(), p));
    return image.rgba_at(p);
  }

  inline RGBA& checked_rgba_at(Image& image, const Point& p) {
    check(containing(image.bounds(), p));
    return image.rgba_at(p);
  }

  template <typename P>
  bool all_of(const Image& image, const Rect& rect, P&& predicate) {
    check_rect(image, rect);
    const auto rgba = image.rgba();
    for (auto y = rect.y; y < rect.y + rect.h; ++y)
      for (auto x = rect.x; x < rect.x + rect.w; ++x)
        if (!predicate(rgba[y * image.width() + x]))
          return false;
    return true;
  }

  template <typename F>
  void for_each_pixel(const Image& image, Rect rect, F&& func) {
    for (auto y = 0; y < rect.h; ++y) {
      auto row = image.rgba() + image.width() * (rect.y + y) + rect.x;
      for (auto x = 0; x < rect.w; ++x, ++row)
        func(*row);
    }
  }

  void blend(Image& image, int x, int y, const RGBA& color) {
    auto& a = image.rgba_at({ x, y });
    const auto& b = color;
    a.r = static_cast<uint8_t>((a.r * (255 - b.a) + b.r * b.a) / 255);
    a.g = static_cast<uint8_t>((a.g * (255 - b.a) + b.g * b.a) / 255);
    a.b = static_cast<uint8_t>((a.b * (255 - b.a) + b.b * b.a) / 255);
    a.a = std::max(a.a, b.a);
  }

  template <int sides, // 4 or 8
    typename Color,
    typename GetPixel, // Color&(int x, int y)
    typename Match, // bool Match(Color color)
    typename Count> // void Count(int x, int y)
  void flood_fill(
      int start_x, int start_y,
      int w, int h,
      const Color& new_color,
      GetPixel&& get_pixel,
      Match&& match,
      Count&& count = [](int, int) {}) {

    // cancel when new color is indistinguishable from rest
    if (match(new_color))
      return;

    auto stack = std::vector<Point>();
    const auto add = [&](auto x, auto y) {
      auto& pixel = get_pixel(x, y);
      if (match(pixel)) {
        pixel = new_color;
        stack.push_back(Point{ x, y });
      }
    };
    add(start_x, start_y);

    while (!stack.empty()) {
      const auto [x, y] = stack.back();
      stack.pop_back();
      count(x, y);

      if (y > 0) add(x, y - 1);
      if (y < h - 1) add(x, y + 1);
      if (sides == 4) {
        if (x > 0) add(x - 1, y);
        if (x < w - 1) add(x + 1, y);
      }
      else {
        if (x > 0) {
          add(x - 1, y);
          if (y > 0) add(x - 1, y - 1);
          if (y < h - 1) add(x - 1, y + 1);
        }
        if (x < w - 1) {
          add(x + 1, y);
          if (y > 0) add(x + 1, y - 1);
          if (y < h - 1) add(x + 1, y + 1);
        }
      }
    }
  }

  void merge_adjacent_rects(const Image& image, std::vector<Rect>& rects,
      int distance, bool gray_levels) {

    const auto adjacent = [&](const Rect& a, const Rect& b) {
      const auto intersection = intersect(a, expand(b, distance));
      if (empty(intersection))
        return false;
      if (gray_levels)
        return !is_fully_black(image, 1, intersection);
      return !is_fully_transparent(image, 1, intersection);
    };

    for (;;) {
      auto merged = false;
      for (auto i = size_t{ }; i < rects.size(); ++i) {
        auto& a = rects[i];
        for (auto j = i + 1; j < rects.size(); ) {
          auto& b = rects[j];
          if (adjacent(a, b)) {
            a = combine(a, b);
            b = rects.back();
            rects.pop_back();
            merged = true;
          }
          else {
            ++j;
          }
        }
      }
      if (!merged)
        break;
    }
  }

  // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
  template<typename F>
  void bresenham_line(int x0, int y0, int x1, int y1, F&& func, bool omit_last) {
    const auto dx = std::abs(x1 - x0);
    const auto sx = (x0 < x1 ? 1 : -1);
    const auto dy = -std::abs(y1 - y0);
    const auto sy = (y0 < y1 ? 1 : -1);
    omit_last = omit_last && (dx || dy);

    for (auto err = dx + dy;;) {
      const auto last = (x0 == x1 && y0 == y1);

      if (!last || !omit_last)
        func(x0, y0);

      if (last)
        break;

      const auto e2 = 2 * err;
      if (e2 >= dy) {
        err += dy;
        x0 += sx;
      }
      if (e2 <= dx) {
        err += dx;
        y0 += sy;
      }
    }
  }

  // http://paulbourke.net/geometry/polygonmesh/
  bool point_in_polygon(real x, real y, const std::vector<PointF>& p) {
    auto c = false;
    for (auto i = size_t{ }, j = p.size() - 1; i < p.size(); j = i++) {
      if ((((p[i].y <= y) && (y < p[j].y)) ||
           ((p[j].y <= y) && (y < p[i].y))) &&
          (x < (p[j].x - p[i].x) * (y - p[i].y) / (p[j].y - p[i].y) + p[i].x))
        c = !c;
    }
    return c;
  }

  // https://en.wikipedia.org/wiki/Median_cut
  std::vector<RGBA> median_cut_reduction(RGBASpan image, int max_colors) {
    struct Bucket {
      uint8_t max_channel_range;
      RGBASpan colors;
    };
  
    auto buckets = std::vector<Bucket>();
    const auto insert_bucket = [&](RGBASpan colors) {
      // compute channel with maximum range
      auto max_channel = 0;
      auto max_channel_range = uint8_t{ };
      for (auto i = 0; i < 4; ++i) {
        const auto [min, max] = std::minmax_element(colors.begin(), colors.end(),
          [&](const RGBA& a, const RGBA& b) { return a.channel(i) < b.channel(i); });
        const auto channel_range = max->channel(i) - min->channel(i);
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

    while (static_cast<int>(buckets.size()) < max_colors) {
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
          sum[i] += color.channel(i);
      auto color = RGBA{ };
      for (auto i = 0; i < 4; ++i)
        color.channel(i) = sum[i] / bucket.colors.size();
      palette.push_back(color);
    }
    return palette;
  }

  int index_of_closest_palette_color(const Palette& palette, const RGBA& color) {
    auto min_index = 0;
    auto min_distance = std::numeric_limits<int>::max();
    for (auto i = 0; i < palette.size(); ++i) {
      const auto r = palette[i].r - color.r;
      const auto g = palette[i].g - color.g;
      const auto b = palette[i].b - color.b;
      const auto distance = (r * r + g * g + b * b);
      if (distance < min_distance) {
        min_index = i;
        min_distance = distance;
      }
    }
    return min_index;
  }

  const RGBA& closest_palette_color(const Palette& palette, const RGBA& color) {
    return palette[index_of_closest_palette_color(palette, color)];
  }

  // https://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
  Image floyd_steinberg_dithering(Image image, const Palette& palette) {
    const auto diff = [](const uint8_t& a, const uint8_t& b) { 
      return static_cast<int>(a) - static_cast<int>(b);
    };
    const auto saturate = [](int value) { 
      return static_cast<uint8_t>(std::clamp(value, 0, 255));
    };
    const auto w = image.width();
    const auto h = image.height();
    for (auto y = 0; y < h; ++y)
      for (auto x = 0; x < w; ++x) {
        auto& color = image.rgba_at({ x, y });
        const auto old_color = color;
        color = closest_palette_color(palette, color);
        const auto error_r = diff(old_color.r, color.r);
        const auto error_g = diff(old_color.g, color.g);
        const auto error_b = diff(old_color.b, color.b);
        const auto apply_error = [&](int x, int y, int fs) {
          auto& color = image.rgba_at({ 
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
    return image;
  }

  // https://giflib.sourceforge.net/whatsinagif/
  bool write_gif(const std::string& filename, const Animation& animation) {
    if (animation.frames.empty())
      return false;
    const auto& first_image = animation.frames.front().image;

    const auto palette = generate_palette(animation, 
      (animation.max_colors ? std::min(animation.max_colors, 256) : 256));
    auto bits = 0;
    for (auto c = palette.size() - 1; c; c >>= 1)
      ++bits;
    auto palette_rgb = std::make_unique<uint8_t[]>((1 << bits) * 3);
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

    const auto [width, height] = first_image.bounds().size();
    auto gif = ge_new_gif(filename.c_str(), width, height, 
      palette_rgb.get(), bits, transparent_index, animation.loop_count);
    if (!gif)
      return false;

    for (const auto& frame : animation.frames) {
      const auto delay = std::chrono::duration_cast<
        std::chrono::duration<uint16_t, std::ratio<1, 100>>>(
        std::chrono::duration<real>(frame.duration)).count();
      const auto mono = quantize_image(frame.image, palette, true);
      std::memcpy(gif->frame, mono.data(), width * height);
      ge_add_frame(gif, delay);
    }
    ge_close_gif(gif);
    return true;
  }
} // namespace

Image::Image(int width, int height)
  : m_width(width),
    m_height(height) {

  if (!width || !height)
    throw std::runtime_error("invalid image size");

  const auto size = to_unsigned(m_width * m_height) * sizeof(RGBA);
  m_data = static_cast<RGBA*>(std::malloc(size));
  if (!m_data)
    throw std::bad_alloc();
}

Image::Image(int width, int height, const RGBA& background)
  : Image(width, height) {
  std::fill(m_data, m_data + (m_width * m_height), background);
}

Image::Image(std::filesystem::path path, std::filesystem::path filename)
  : m_path(std::move(path)),
    m_filename(std::move(filename)) {

  const auto full_path = m_path / m_filename;

#if defined(EMBED_TEST_FILES)
  if (full_path == "test/Items.png") {
    unsigned char file[] {
#include "test/Items.png.inc"
    };
    auto channels = 0;
    m_data = reinterpret_cast<RGBA*>(stbi_load_from_memory(
        file, sizeof(file), &m_width, &m_height, &channels, sizeof(RGBA)));
    return;
  }
#endif

#if defined(_WIN32)
  if (auto file = _wfopen(full_path.wstring().c_str(), L"rb")) {
#else
  if (auto file = std::fopen(path_to_utf8(full_path).c_str(), "rb")) {
#endif
    auto channels = 0;
    m_data = reinterpret_cast<RGBA*>(stbi_load_from_file(
        file, &m_width, &m_height, &channels, sizeof(RGBA)));
    std::fclose(file);
  }
  if (!m_data)
    throw std::runtime_error("loading file '" + 
      path_to_utf8(full_path) + "' failed");
}

Image::Image(Image&& rhs)
  : m_path(std::exchange(rhs.m_path, { })),
    m_filename(std::exchange(rhs.m_filename, { })),
    m_data(std::exchange(rhs.m_data, nullptr)),
    m_width(std::exchange(rhs.m_width, 0)),
    m_height(std::exchange(rhs.m_height, 0)) {
}

Image& Image::operator=(Image&& rhs) {
  auto tmp = std::move(rhs);
  std::swap(m_path, tmp.m_path);
  std::swap(m_filename, tmp.m_filename);
  std::swap(m_data, tmp.m_data);
  std::swap(m_width, tmp.m_width);
  std::swap(m_height, tmp.m_height);
  return *this;
}

Image::~Image() {
  stbi_image_free(m_data);
}

MonoImage::MonoImage(int width, int height)
  : m_width(width),
    m_height(height) {

  if (!width || !height)
    throw std::runtime_error("invalid image size");

  const auto size = to_unsigned(m_width * m_height) * sizeof(uint8_t);
  m_data = static_cast<uint8_t*>(std::malloc(size));
  if (!m_data)
    throw std::bad_alloc();
}

MonoImage::MonoImage(int width, int height, Value background)
  : MonoImage(width, height) {
  std::fill(m_data, m_data + (m_width * m_height), background);
}

MonoImage::MonoImage(MonoImage&& rhs)
  : m_data(std::exchange(rhs.m_data, nullptr)),
    m_width(std::exchange(rhs.m_width, 0)),
    m_height(std::exchange(rhs.m_height, 0)) {
}

MonoImage& MonoImage::operator=(MonoImage&& rhs) {
  auto tmp = std::move(rhs);
  std::swap(m_data, tmp.m_data);
  std::swap(m_width, tmp.m_width);
  std::swap(m_height, tmp.m_height);
  return *this;
}

MonoImage::~MonoImage() {
  stbi_image_free(m_data);
}

Image Image::clone(const Rect& rect) const {
  if (empty(rect))
    return clone(bounds());
  check_rect(*this, rect);
  auto clone = Image(rect.w, rect.h);
  copy_rect(*this, rect, clone, 0, 0);
  return clone;
}

void save_image(const Image& image, const std::filesystem::path& path) {
  if (!path.parent_path().empty())
    std::filesystem::create_directories(path.parent_path());
  const auto filename = path_to_utf8(path);
  const auto extension = to_lower(path_to_utf8(path.extension()));
  const auto w = image.width();
  const auto h = image.height();
  const auto comp = sizeof(RGBA);
  const auto data = image.rgba();
  const auto stride = image.width() * to_int(sizeof(RGBA));

  stbi_write_tga_with_rle = 1;
  if (!(extension == ".png" && stbi_write_png(filename.c_str(), w, h, comp, data, stride)) &&
      !(extension == ".bmp" && stbi_write_bmp(filename.c_str(), w, h, comp, data)) &&
      !(extension == ".tga" && stbi_write_tga(filename.c_str(), w, h, comp, data)))
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

void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy) {
  const auto [sx, sy, w, h] = source_rect;
  const auto dest_rect = Rect{ dx, dy, w, h };
  if (source_rect == source.bounds() &&
      dest_rect == dest.bounds() &&
      source_rect == dest_rect) {
    std::memcpy(dest.rgba(), source.rgba(),
      to_unsigned(w * h) * sizeof(RGBA));
  }
  else {
    check_rect(source, source_rect);
    check_rect(dest, dest_rect);
    for (auto y = 0; y < h; ++y)
      std::memcpy(
        dest.rgba() + ((dy + y) * dest.width() + dx),
        source.rgba() + ((sy + y) * source.width() + sx),
        to_unsigned(w) * sizeof(RGBA));
  }
}

void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy) {
  const auto [sx, sy, w, h] = source_rect;
  check_rect(source, source_rect);
  check_rect(dest, { dx, dy, h, w });
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      std::memcpy(
        dest.rgba() + ((dy + x) * dest.width() + (dx + h-1 - y)),
        source.rgba() + ((sy + y) * source.width() + sx + x),
        sizeof(RGBA));
}

void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy,
    const std::vector<PointF>& mask_vertices) {
  const auto [sx, sy, w, h] = source_rect;
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      if (point_in_polygon(x + 0.5, y + 0.5, mask_vertices))
        checked_rgba_at(dest, { dx + x, dy + y }) = checked_rgba_at(source, { sx + x, sy + y });
}

void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy,
    const std::vector<PointF>& mask_vertices) {
  const auto [sx, sy, w, h] = source_rect;
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      if (point_in_polygon(x + 0.5, y + 0.5, mask_vertices))
        checked_rgba_at(dest, { dx + (h-1 - y), dy + x }) = checked_rgba_at(source, { sx + x, sy + y });
}

void extrude_rect(Image& image, const Rect& rect, int count, WrapMode mode,
    bool left, bool top, bool right, bool bottom) {
  check_rect(image, expand(rect, count));

  for (auto i = 1; i <= count; ++i) {
    const auto d = expand(rect, i);
    const auto dx0 = d.x;
    const auto dy0 = d.y;
    const auto dx1 = d.x1() - 1;
    const auto dy1 = d.y1() - 1;

    auto wx = 0;
    auto wy = 0;
    if (mode != WrapMode::clamp) {
      wx = (rect.w - i % rect.w) % rect.w;
      wy = (rect.h - i % rect.h) % rect.h;
      if (mode == WrapMode::mirror) {
        if (((i - 1) / rect.w) % 2 == 0)
          wx = rect.w - 1 - wx;
        if (((i - 1) / rect.h) % 2 == 0)
          wy = rect.h - 1 - wy;
      }
    }
    const auto sx0 = rect.x + wx;
    const auto sy0 = rect.y + wy;
    const auto sx1 = rect.x1() - 1 - wx;
    const auto sy1 = rect.y1() - 1 - wy;

    if (top)
      std::memcpy(&image.rgba_at({ dx0 + 1, dy0 }), 
                  &image.rgba_at({ dx0 + 1, sy0 }),
                  to_unsigned(dx1 - dx0 - 1) * sizeof(RGBA));
    if (bottom)
      std::memcpy(&image.rgba_at({ dx0 + 1, dy1 }), 
                  &image.rgba_at({ dx0 + 1, sy1 }), 
                  to_unsigned(dx1 - dx0 - 1) * sizeof(RGBA));
    if (left)
      for (auto y = dy0; y <= dy1; ++y)
        image.rgba_at({ dx0, y }) = image.rgba_at({ sx0, y });

    if (right)
      for (auto y = dy0; y <= dy1; ++y)
        image.rgba_at({ dx1, y }) = image.rgba_at({ sx1, y });
  }
}

void draw_rect(Image& image, const Rect& rect, const RGBA& color) {
  const auto checked_blend = [&](int x, int y) {
    if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
      blend(image, x, y, color);
  };
  for (auto x = rect.x; x < rect.x + rect.w; ++x) {
    checked_blend(x, rect.y);
    checked_blend(x, rect.y + rect.h - 1);
  }
  for (auto y = rect.y + 1; y < rect.y + rect.h - 1; ++y) {
    checked_blend(rect.x, y);
    checked_blend(rect.x + rect.w - 1, y);
  }
}

void draw_line(Image& image, const Point& p0, const Point& p1, const RGBA& color, bool omit_last) {
  const auto checked_blend = [&](int x, int y) {
    if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
      blend(image, x, y, color);
  };
  bresenham_line(p0.x, p0.y, p1.x, p1.y, checked_blend, omit_last);
}

void draw_line_stipple(Image& image, const Point& p0, const Point& p1, const RGBA& color, int stipple, bool omit_last) {
  int i = 0;
  const auto checked_blend = [&](int x, int y) {
    if (Point(x, y) == p0 || Point(x, y) == p1)
      i = 0;
    if ((i++ / stipple) % 2 == 0)
      if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
        blend(image, x, y, color);
  };
  bresenham_line(p0.x, p0.y, p1.x, p1.y, checked_blend, omit_last);
}

void draw_rect_stipple(Image& image, const Rect& rect, const RGBA& color, int stipple) {
  const auto points = std::array{
    Point(rect.x0(), rect.y0()),
    Point(rect.x1() - 1, rect.y0()),
    Point(rect.x1() - 1, rect.y1() - 1),
    Point(rect.x0(), rect.y1() - 1)
  };
  for (auto i = size_t{ }; i < 4; ++i)
    draw_line_stipple(image, points[i], points[(i + 1) % 4], color, stipple, true);
}

void fill_rect(Image& image, const Rect& rect, const RGBA& color) {
  check_rect(image, rect);
  for (auto y = rect.y; y < rect.y + rect.h; ++y)
    for (auto x = rect.x; x < rect.x + rect.w; ++x)
      blend(image, x, y, color);
}

bool is_opaque(const Image& image, const Rect& rect) {
  if (empty(rect))
    return is_opaque(image, image.bounds());

  return all_of(image, rect, [](const RGBA& rgba) { return (rgba.a == 255); });
}

bool is_fully_transparent(const Image& image, int threshold, const Rect& rect) {
  if (empty(rect))
    return is_fully_transparent(image, threshold, image.bounds());

  return all_of(image, rect, [&](const RGBA& rgba) { return (rgba.a < threshold); });
}

bool is_fully_black(const Image& image, int threshold, const Rect& rect) {
  if (empty(rect))
    return is_fully_black(image, threshold, image.bounds());

  return all_of(image, rect, [&](const RGBA& rgba) { return (rgba.gray() < threshold); });
}

bool is_identical(const Image& image_a, const Rect& rect_a, const Image& image_b, const Rect& rect_b) {
  check_rect(image_a, rect_a);
  check_rect(image_b, rect_b);
  if (rect_a.w != rect_b.w || rect_a.h != rect_b.h)
    return false;

  for (auto y = 0; y < rect_a.h; ++y)
    if (std::memcmp(
        image_a.rgba() + (rect_a.y + y) * image_a.width() + rect_a.x,
        image_b.rgba() + (rect_b.y + y) * image_b.width() + rect_b.x,
        to_unsigned(rect_a.w) * sizeof(RGBA)))
      return false;

  return true;
}

Rect get_used_bounds(const Image& image, bool gray_levels, int threshold, const Rect& rect) {
  if (empty(rect))
    return get_used_bounds(image, gray_levels, threshold, image.bounds());

  const auto x1 = rect.x + rect.w - 1;
  const auto y1 = rect.y + rect.h - 1;

  const auto check = (gray_levels ? is_fully_black : is_fully_transparent);

  auto min_y = rect.y;
  for (; min_y < y1; ++min_y)
    if (!check(image, threshold, { rect.x, min_y, rect.w, 1 }))
      break;

  auto max_y = y1;
  for (; max_y > min_y; --max_y)
    if (!check(image, threshold, { rect.x, max_y, rect.w, 1 }))
      break;

  auto min_x = rect.x;
  for (; min_x < x1; ++min_x)
    if (!check(image, threshold, { min_x, min_y, 1, max_y - min_y + 1 }))
      break;

  auto max_x = x1;
  for (; max_x > min_x; --max_x)
    if (!check(image, threshold, { max_x, min_y, 1, max_y - min_y + 1 }))
      break;

  return { min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 };
}

RGBA guess_colorkey(const Image& image) {
  // simply take median of corner colors
  const auto corners = {
    Point{ image.width() - 1, image.height() - 1 },
    Point{ image.width() - 1, 0 },
    Point{ 0, image.height() - 1  },
    Point{ 0, 0 },
  };
  auto colors = std::vector<RGBA>();
  for (const auto& corner : corners)
    colors.push_back(image.rgba_at(corner));
  std::sort(begin(colors), end(colors),
    [](const RGBA& a, const RGBA& b) { return a.rgba < b.rgba; });
  return colors[1];
}

void replace_color(Image& image, RGBA original, RGBA color) {
  if (original != color)
    std::replace(image.rgba(), image.rgba() + image.width() * image.height(),
      original, color);
}

std::vector<Rect> find_islands(const Image& image, int merge_distance,
    bool gray_levels, const Rect& rect) {
  if (empty(rect))
    return find_islands(image, merge_distance, gray_levels,
      get_used_bounds(image, gray_levels));

  using Value = MonoImage::Value;
  auto levels = (gray_levels ?
    get_gray_levels(image, rect) :
    get_alpha_levels(image, rect));

  auto islands = std::vector<Rect>();
  for (auto y = 0; y < rect.h; ++y)
    for (auto x = 0; x < rect.w; ++x)
      if (levels.value_at({ x, y })) {
        auto min_x = x;
        auto min_y = y;
        auto max_x = x;
        auto max_y = y;
        flood_fill<8>(x, y, rect.w, rect.h,
          Value{ },
          [&](int x, int y) -> Value& { return levels.value_at({ x, y }); },
          [&](const Value& pixel) { return (pixel != 0); },
          [&](int x, int y) {
            min_x = std::min(x, min_x);
            min_y = std::min(y, min_y);
            max_x = std::max(x, max_x);
            max_y = std::max(y, max_y);
          });
        islands.push_back({ rect.x + min_x, rect.y + min_y, max_x - min_x + 1, max_y - min_y + 1 });
      }

  merge_adjacent_rects(image, islands, merge_distance, gray_levels);

  // fuzzy sort from top to bottom, left to right
  const auto center_considerably_less = [](const Rect& a, const Rect& b) {
    const auto row_tolerance = std::min(a.h, b.h) / 4;
    const auto ca = a.center();
    const auto cb = b.center();
    if (ca.y < cb.y - row_tolerance) return true;
    if (cb.y < ca.y - row_tolerance) return false;
    return std::tie(ca.x, ca.y) < std::tie(cb.x, cb.y);
  };
  std::stable_sort(begin(islands), end(islands), center_considerably_less);

  return islands;
}

void clear_alpha(Image& image) {
  std::for_each(image.rgba(), image.rgba() + image.width() * image.height(),
    [](RGBA& rgba) {
      if (rgba.a == 0)
        rgba = RGBA{ };
    });
}

void make_opaque(Image& image, RGBA background) {
  std::for_each(image.rgba(), image.rgba() + image.width() * image.height(),
    [&](RGBA& rgba) {
      if (rgba.a == 0)
        rgba = background;
      else
        rgba.a = 255;
    });
}

void premultiply_alpha(Image& image) {
  const auto multiply = [](int channel, int alpha) {
    return static_cast<uint8_t>(channel * alpha / 256);
  };
  const auto size = image.width() * image.height();
  auto rgba = image.rgba();
  for (auto i = 0; i < size; ++i, ++rgba) {
    rgba->r = multiply(rgba->r, rgba->a);
    rgba->g = multiply(rgba->g, rgba->a);
    rgba->b = multiply(rgba->b, rgba->a);
  }
}

void bleed_alpha(Image& image) {
  bleed_apply(reinterpret_cast<uint8_t*>(image.rgba()),
    image.width(), image.height());
}

MonoImage get_alpha_levels(const Image& image, const Rect& rect) {
  if (empty(rect))
    return get_alpha_levels(image, get_used_bounds(image, false));
  check_rect(image, rect);

  auto result = MonoImage(rect.w, rect.h);
  auto dest = result.data();
  for_each_pixel(image, rect, [&](const RGBA& color) { *dest++ = color.a; });
  return result;
}

MonoImage get_gray_levels(const Image& image, const Rect& rect) {
  if (empty(rect))
    return get_gray_levels(image, get_used_bounds(image, true));
  check_rect(image, rect);

  auto result = MonoImage(rect.w, rect.h);
  auto dest = result.data();
  for_each_pixel(image, rect, [&](const RGBA& color) { *dest++ = color.gray(); });
  return result;
}

Image resize_image(const Image& image, real scale, ResizeFilter filter) {
  const auto width = to_int(image.width() * scale + 0.5);
  const auto height = to_int(image.height() * scale + 0.5);
  if (width == image.width() && height == image.height())
    return image.clone();
  auto output = Image(width, height);
  const auto flags = 0;
  const auto edge_mode = STBIR_EDGE_CLAMP;
  const auto color_space = STBIR_COLORSPACE_SRGB;
  const auto bytes_per_pixel = int{ sizeof(RGBA) };
  if (!stbir_resize(image.rgba(), image.width(), image.height(), image.width() * bytes_per_pixel,
        output.rgba(), width, height, width * bytes_per_pixel,
        STBIR_TYPE_UINT8, 4, 3, flags, edge_mode, edge_mode, 
        static_cast<stbir_filter>(filter), static_cast<stbir_filter>(filter), 
        color_space, nullptr))
    throw std::runtime_error("resizing image failed");
  return output;
}

Palette generate_palette(const Image& image_, int count) {
  auto image = image_.clone();
  const auto size = static_cast<size_t>(image.width() * image.height());
  return median_cut_reduction({ image.rgba(), size }, count);
}

Palette generate_palette(const Animation& animation, int count) {
  auto merged_size = size_t{ };
  for (const auto& frame : animation.frames)
    merged_size += static_cast<size_t>(frame.image.width() * frame.image.height());
  auto merged = std::make_unique<RGBA[]>(merged_size);
  auto pos = merged.get();
  for (const auto& frame : animation.frames) {
    const auto size = static_cast<size_t>(frame.image.width() * frame.image.height());
    std::memcpy(pos, frame.image.rgba(), size * sizeof(RGBA));
    pos += size;
  }
  return median_cut_reduction({ merged.get(), merged_size }, count);
}

MonoImage quantize_image(const Image& image, const Palette& palette, bool dither) {
  if (dither)
    return quantize_image(floyd_steinberg_dithering(
      image.clone(), palette), palette, false);

  auto out = MonoImage(image.width(), image.height());
  for (auto y = 0; y < image.height(); ++y)
    for (auto x = 0; x < image.width(); ++x)
      out.value_at({ x, y }) = 
        index_of_closest_palette_color(palette, image.rgba_at({ x, y }));
  return out;
}

Image apply_palette(const MonoImage& image, const Palette& palette) {
  auto out = Image(image.width(), image.height());
  const auto max = static_cast<uint8_t>(palette.size() - 1);
  for (auto y = 0; y < image.height(); ++y)
    for (auto x = 0; x < image.width(); ++x)
      out.rgba_at({ x, y }) = palette[std::min(max, image.value_at({ x, y }))];
  return out;
}

} // namespace
