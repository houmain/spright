#pragma once

#include <tuple>

namespace spright {

template<typename T>
struct SizeT {
  T x{ };
  T y{ };

  SizeT() = default;
  SizeT(T x, T y)
    : x(x), y(y) {
  }
  template<typename S>
  explicit SizeT(const SizeT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)) {
  }
};

template<typename T>
struct PointT {
  T x{ };
  T y{ };

  PointT() = default;
  PointT(T x, T y)
    : x(x), y(y) {
  }
  template<typename S>
  explicit PointT(const SizeT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)) {
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
  RectT(T x, T y, T w, T h)
    : x(x), y(y), w(w), h(h) {
  }
  template<typename S>
  explicit RectT(const RectT<S>& s)
    : x(static_cast<T>(s.x)),
      y(static_cast<T>(s.y)),
      w(static_cast<T>(s.w)),
      h(static_cast<T>(s.h)) {
  }

  T x0() const { return x; }
  T y0() const { return y; }
  T x1() const { return x + w; }
  T y1() const { return y + h; }
  Point xy() const { return { x, y }; }
  Point center() const { return { x + w / 2, y + h / 2 }; }
  Size size() const { return { w, h }; }

  friend bool operator==(const RectT& a, const RectT& b) {
    return std::tie(a.x, a.y, a.w, a.h) == std::tie(b.x, b.y, b.w, b.h);
  }
  friend bool operator!=(const RectT& a, const RectT& b) { return !(a == b); }
};

using Size = SizeT<int>;
using SizeF = SizeT<float>;
using Point = PointT<int>;
using PointF = PointT<float>;
using Rect = RectT<int>;
using RectF = RectT<float>;

inline bool empty(const Size& size) { return (size.x == 0 || size.y == 0); }
inline bool empty(const Rect& rect) { return (rect.w == 0 || rect.h == 0); }

Rect expand(const Rect& rect, int value);
Rect intersect(const Rect& a, const Rect& b);
bool containing(const Rect& a, const Rect& b);
bool containing(const Rect& a, const Point& p);
bool overlapping(const Rect& a, const Rect& b);
Rect combine(const Rect& a, const Rect& b);

} // namespace
