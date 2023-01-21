
#include "Rect.h"
#include <algorithm>

namespace spright {

Rect expand(const Rect& rect, int value) {
  return { rect.x - value, rect.y - value, rect.w + value * 2, rect.h + value * 2 };
}

Rect intersect(const Rect& a, const Rect& b) {
  const auto x0 = std::max(a.x0(), b.x0());
  const auto y0 = std::max(a.y0(), b.y0());
  const auto x1 = std::min(a.x1(), b.x1());
  const auto y1 = std::min(a.y1(), b.y1());
  return { x0, y0, std::max(x1 - x0, 0), std::max(y1 - y0, 0) };
}

bool containing(const Rect& a, const Rect& b) {
  return (a.x <= b.x &&
          a.y <= b.y &&
          a.x + a.w >= b.x + b.w &&
          a.y + a.h >= b.y + b.h);
}

bool containing(const Rect& a, const Point& p) {
  return (a.x <= p.x &&
          a.y <= p.y &&
          a.x + a.w > p.x &&
          a.y + a.h > p.y);
}

bool overlapping(const Rect& a, const Rect& b) {
  return !(a.x + a.w <= b.x ||
           b.x + b.w <= a.x ||
           a.y + a.h <= b.y ||
           b.y + b.h <= a.y);
}

Rect combine(const Rect& a, const Rect& b) {
  const auto x0 = std::min(a.x0(), b.x0());
  const auto y0 = std::min(a.y0(), b.y0());
  const auto x1 = std::max(a.x1(), b.x1());
  const auto y1 = std::max(a.y1(), b.y1());
  return { x0, y0, x1 - x0, y1 - y0 };
}

} // namespace
