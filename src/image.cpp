
#include "image.h"
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace {
  void check_rect(const Image& image, const Rect& rect) {
    const auto [x, y, w, h] = rect;
    if (x < 0 ||
        y < 0 ||
        x + w > image.width() ||
        y + h > image.height())
      throw std::logic_error("access outside image bounds");
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

  template <int sides, // 4 or 8
    typename Match, // bool Match(RGBA)
    typename Count> // void Count(int x, int y)
  void flood_fill(Image& image, const Rect& bounds,
      const Point& start, const RGBA& new_color,
      Match&& match, Count&& count = [](int, int) {}) {

    // cancel when new color is indistinguishable from rest
    if (match(new_color))
      return;

    auto stack = std::vector<Point>();
    const auto add = [&](auto x, auto y) {
      auto& pixel = image.rgba_at({ x, y });
      if (match(pixel)) {
        pixel = new_color;
        stack.push_back(Point{ x, y });
      }
    };
    add(start.x, start.y);

    while (!stack.empty()) {
      const auto pos = stack.back();
      stack.pop_back();
      count(pos.x, pos.y);

      if (pos.y > bounds.y)     add(pos.x, pos.y - 1);
      if (pos.y < bounds.h - 1) add(pos.x, pos.y + 1);
      if (sides == 4) {
        if (pos.x > bounds.x)     add(pos.x - 1, pos.y);
        if (pos.x < bounds.w - 1) add(pos.x + 1, pos.y);
      }
      else {
        if (pos.x > bounds.x) {
          add(pos.x - 1, pos.y);
          if (pos.y > bounds.y)     add(pos.x - 1, pos.y - 1);
          if (pos.y < bounds.h - 1) add(pos.x - 1, pos.y + 1);
        }
        if (pos.x < bounds.w - 1) {
          add(pos.x + 1, pos.y);
          if (pos.y > bounds.y)     add(pos.x + 1, pos.y - 1);
          if (pos.y < bounds.h - 1) add(pos.x + 1, pos.y + 1);
        }
      }
    }
  }
} // namespace

Image::Image(int width, int height, const RGBA& background)
  : m_data(static_cast<RGBA*>(std::malloc(static_cast<size_t>(width * height * 4)))),
    m_width(width),
    m_height(height) {

  std::fill(m_data, m_data + (m_width * m_height), background);
}

Image::Image(std::filesystem::path filename)
  : m_filename(std::move(filename)) {

  if (auto file = std::fopen(path_to_utf8(m_filename).c_str(), "rb")) {
    auto channels = 0;
    m_data = reinterpret_cast<RGBA*>(stbi_load_from_file(
        file, &m_width, &m_height, &channels, 4));
    std::fclose(file);
  }
  if (!m_data)
    throw std::runtime_error("loading file '" + path_to_utf8(m_filename) + "' failed");
}

Image::Image(Image&& rhs)
  : m_filename(std::exchange(rhs.m_filename, { })),
    m_data(std::exchange(rhs.m_data, nullptr)),
    m_width(std::exchange(rhs.m_width, 0)),
    m_height(std::exchange(rhs.m_height, 0)) {
}

Image& Image::operator=(Image&& rhs) {
  auto tmp = std::move(rhs);
  std::swap(m_filename, tmp.m_filename);
  std::swap(m_data, tmp.m_data);
  std::swap(m_width, tmp.m_width);
  std::swap(m_height, tmp.m_height);
  return *this;
}

Image::~Image() {
  stbi_image_free(m_data);
}

Image Image::clone() const {
  auto clone = Image(width(), height());
  std::memcpy(clone.rgba(), rgba(), static_cast<size_t>(width() * height()) * 4);
  return clone;
}

void save_image(const Image& image, const std::filesystem::path& filename) {
  if (!stbi_write_png(path_to_utf8(filename).c_str(), image.width(), image.height(), 4,
      image.rgba(), image.width() * 4))
    throw std::runtime_error("writing file '" + path_to_utf8(filename) + "' failed");
}

void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy) {
  const auto [sx, sy, w, h] = source_rect;
  check_rect(source, source_rect);
  check_rect(dest, { dx, dy, w, h });
  for (auto y = 0; y < h; ++y)
    std::memcpy(
      dest.rgba() + ((dy + y) * dest.width() + dx),
      source.rgba() + ((sy + y) * source.width() + sx),
      static_cast<size_t>(w) * sizeof(RGBA));
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

void draw_rect(Image& image, const Rect& rect, const RGBA& color) {
  const auto blend = [&](int x, int y) {
    if (x >= 0 && x < image.width() && y >= 0 && y < image.height()) {
      auto& a = image.rgba_at({ x, y });
      const auto& b = color;
      a.r = static_cast<uint8_t>((a.r * (255 - b.a) + b.r * b.a) / 255);
      a.g = static_cast<uint8_t>((a.g * (255 - b.a) + b.g * b.a) / 255);
      a.b = static_cast<uint8_t>((a.b * (255 - b.a) + b.b * b.a) / 255);
      a.a = std::max(a.a, b.a);
    }
  };
  for (auto x = rect.x; x < rect.x + rect.w; ++x) {
    blend(x, rect.y);
    blend(x, rect.y + rect.h - 1);
  }
  for (auto y = rect.y + 1; y < rect.y + rect.h - 1; ++y) {
    blend(rect.x, y);
    blend(rect.x + rect.w - 1, y);
  }
}

bool is_opaque(const Image& image, const Rect& rect) {
  if (empty(rect))
    return is_opaque(image, image.bounds());

  return all_of(image, rect, [](const RGBA& rgba) { return (rgba.a == 255); });
}

bool is_fully_transparent(const Image& image, const Rect& rect) {
  if (empty(rect))
    return is_fully_transparent(image, image.bounds());

  return all_of(image, rect, [](const RGBA& rgba) { return (rgba.a == 0); });
}

Rect get_used_bounds(const Image& image, const Rect& rect) {
  if (empty(rect))
    return get_used_bounds(image, image.bounds());

  const auto x1 = rect.x + rect.w - 1;
  const auto y1 = rect.y + rect.h - 1;

  auto min_y = rect.y;
  for (; min_y < y1; ++min_y)
    if (!is_fully_transparent(image, { rect.x, min_y, rect.w, 1 }))
      break;

  auto max_y = y1;
  for (; max_y > min_y; --max_y)
    if (!is_fully_transparent(image, { rect.x, max_y, rect.w, 1 }))
      break;

  auto min_x = rect.x;
  for (; min_x < x1; ++min_x)
    if (!is_fully_transparent(image, { min_x, min_y, 1, max_y - min_y }))
      break;

  auto max_x = x1;
  for (; max_x > min_x; --max_x)
    if (!is_fully_transparent(image, { max_x, min_y, 1, max_y - min_y }))
      break;

  return { min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 };
}

RGBA guess_colorkey(const Image& image) {
  // find a corner within larger same-color area using flood fill
  const auto limit = std::max(image.height(), image.height());

  const auto corners = {
    Point{ image.width() - 1, image.height() - 1 },
    Point{ image.width() - 1, 0 },
    Point{ 0, image.height() - 1  },
    Point{ 0, 0 },
  };
  auto clone = image.clone();
  for (const auto& corner : corners) {
    const auto color = image.rgba_at(corner);
    auto count = 0;
    flood_fill<4>(clone, clone.bounds(), corner, RGBA{ },
      [&](const RGBA& pixel) { return (pixel == color); },
      [&](int, int) { ++count; });
    if (count > limit)
      return color;
  }
  return { };
}

void replace_color(Image& image, RGBA original, RGBA color) {
  if (original != color)
    std::replace(image.rgba(), image.rgba() + image.width() * image.height(),
      original, color);
}

std::vector<Rect> find_islands(const Image& image, const Rect& rect) {
  if (empty(rect))
    return find_islands(image, get_used_bounds(image));

  auto clone = image.clone();

  auto islands = std::vector<Rect>();
  for (auto y = rect.y; y < rect.y + rect.h; ++y)
    for (auto x = rect.x; x < rect.x + rect.w; ++x)
      if (clone.rgba_at({ x, y }).a) {
        auto min_x = x;
        auto min_y = y;
        auto max_x = x;
        auto max_y = y;
        flood_fill<8>(clone, clone.bounds(), { x, y }, RGBA{ },
          [&](const RGBA& pixel) { return (pixel.a != 0); },
          [&](int x, int y) {
            min_x = std::min(x, min_x);
            min_y = std::min(y, min_y);
            max_x = std::max(x, max_x);
            max_y = std::max(y, max_y);
          });
        islands.push_back({ min_x, min_y, max_x - min_x + 1, max_y - min_y + 1 });
      }
  return islands;
}
