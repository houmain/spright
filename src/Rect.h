#pragma once

#include <algorithm>
#include <tuple>

namespace spright {

using real = double;

template<typename T>
struct SizeT {
  T x{ };
  T y{ };

  SizeT() = default;
  constexpr SizeT(T x, T y)
    : x(x), y(y) {
  }
  template<typename S>
  constexpr explicit SizeT(const SizeT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)) {
  }

  friend bool operator==(const SizeT& a, const SizeT& b) {
    return (a.x == b.x && a.y == b.y);
  }
  friend SizeT operator-(const SizeT& a, const SizeT& b) {
    return SizeT(a.x - b.x, a.y - b.y);
  }
};

template<typename T>
struct PointT {
  T x{ };
  T y{ };

  PointT() = default;
  constexpr PointT(T x, T y)
    : x(x), y(y) {
  }
  template<typename S>
  constexpr explicit PointT(const PointT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)) {
  }

  friend bool operator==(const PointT& a, const PointT& b) {
    return (a.x == b.x && a.y == b.y);
  }
  friend PointT operator+(const PointT& a, const PointT& b) {
    return { a.x + b.x, a.y + b.y };
  }
  friend PointT operator-(const PointT& a, const PointT& b) {
    return { a.x - b.x, a.y - b.y };
  }
};

template<typename T>
struct RectT {
  using Point = PointT<T>;
  using Size = SizeT<T>;

  T x{ };
  T y{ };
  T w{ };
  T h{ };

  RectT() = default;
  constexpr RectT(T x, T y, T w, T h)
    : x(x), y(y), w(w), h(h) {
  }
  template<typename S>
  constexpr explicit RectT(const RectT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)),
      w(static_cast<T>(s.w)),
      h(static_cast<T>(s.h)) {
  }

  constexpr T x0() const { return x; }
  constexpr T y0() const { return y; }
  constexpr T x1() const { return x + w; }
  constexpr T y1() const { return y + h; }
  constexpr Point xy() const { return { x, y }; }
  constexpr Point center() const { return { x + w / 2, y + h / 2 }; }
  constexpr Size size() const { return { w, h }; }

  friend bool operator==(const RectT& a, const RectT& b) {
    return std::tie(a.x, a.y, a.w, a.h) == std::tie(b.x, b.y, b.w, b.h);
  }
  friend bool operator!=(const RectT& a, const RectT& b) { return !(a == b); }
};

using Size = SizeT<int>;
using SizeF = SizeT<real>;
using Point = PointT<int>;
using PointF = PointT<real>;
using Rect = RectT<int>;
using RectF = RectT<real>;

constexpr bool empty(const Size& size) { return (size.x == 0 || size.y == 0); }
constexpr bool empty(const Rect& rect) { return (rect.w == 0 || rect.h == 0); }

constexpr PointF rotate_cw(const PointF& point, real width) {
  return { width - point.y, point.x };
}

constexpr Rect expand(const Rect& rect, int value) {
  return { rect.x - value, rect.y - value, rect.w + value * 2, rect.h + value * 2 };
}

constexpr Rect intersect(const Rect& a, const Rect& b) {
  const auto x0 = std::max(a.x0(), b.x0());
  const auto y0 = std::max(a.y0(), b.y0());
  const auto x1 = std::min(a.x1(), b.x1());
  const auto y1 = std::min(a.y1(), b.y1());
  return { x0, y0, std::max(x1 - x0, 0), std::max(y1 - y0, 0) };
}

constexpr bool containing(const Rect& a, const Rect& b) {
  return (a.x <= b.x &&
          a.y <= b.y &&
          a.x + a.w >= b.x + b.w &&
          a.y + a.h >= b.y + b.h);
}

constexpr bool containing(const Rect& a, const Point& p) {
  return (a.x <= p.x &&
          a.y <= p.y &&
          a.x + a.w > p.x &&
          a.y + a.h > p.y);
}

constexpr bool overlapping(const Rect& a, const Rect& b) {
  return !(a.x + a.w <= b.x ||
           b.x + b.w <= a.x ||
           a.y + a.h <= b.y ||
           b.y + b.h <= a.y);
}

constexpr Rect combine(const Rect& a, const Rect& b) {
  const auto x0 = std::min(a.x0(), b.x0());
  const auto y0 = std::min(a.y0(), b.y0());
  const auto x1 = std::max(a.x1(), b.x1());
  const auto y1 = std::max(a.y1(), b.y1());
  return { x0, y0, x1 - x0, y1 - y0 };
}

} // namespace
