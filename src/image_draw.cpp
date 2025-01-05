
#include "image.h"
#include <array>
#include <algorithm>

namespace spright {

namespace {
  void blend(ImageView<RGBA> image_rgba, int x, int y, const RGBA& color) {
    auto& a = image_rgba.value_at({ x, y });
    const auto& b = color;
    a.r = RGBA::to_channel((a.r * (255 - b.a) + b.r * b.a) / 255);
    a.g = RGBA::to_channel((a.g * (255 - b.a) + b.g * b.a) / 255);
    a.b = RGBA::to_channel((a.b * (255 - b.a) + b.b * b.a) / 255);
    a.a = std::max(a.a, b.a);
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
} // namespace

void draw_rect(Image& image, const Rect& rect, const RGBA& color) {
  const auto image_rgba = image.view<RGBA>();
  const auto checked_blend = [&](int x, int y) {
    if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
      blend(image_rgba, x, y, color);
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
  const auto image_rgba = image.view<RGBA>();
  const auto checked_blend = [&](int x, int y) {
    if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
      blend(image_rgba, x, y, color);
  };
  bresenham_line(p0.x, p0.y, p1.x, p1.y, checked_blend, omit_last);
}

void draw_line_stipple(Image& image, const Point& p0, const Point& p1, const RGBA& color, int stipple, bool omit_last) {
  const auto image_rgba = image.view<RGBA>();
  auto i = 0;
  const auto checked_blend = [&](int x, int y) {
    if (Point(x, y) == p0 || Point(x, y) == p1)
      i = 0;
    if ((i++ / stipple) % 2 == 0)
      if (x >= 0 && x < image.width() && y >= 0 && y < image.height())
        blend(image_rgba, x, y, color);
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
  const auto image_rgba = image.view<RGBA>();
  for (auto y = rect.y; y < rect.y + rect.h; ++y)
    for (auto x = rect.x; x < rect.x + rect.w; ++x)
      blend(image_rgba, x, y, color);
}

} // namespace
