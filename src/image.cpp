
#include "image.h"
#include "stb/stb_image_resize2.h"
#include <array>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <utility>

#define TEXBLEED_IMPLEMENTATION
#include "rmj/rmj_texbleed.h"

namespace spright {

namespace {
  const RGBA& checked_rgba_at(ImageView<const RGBA> image_rgba, const Point& p) {
    check(containing(image_rgba.bounds(), p));
    return image_rgba.value_at(p);
  }

  RGBA& checked_rgba_at(ImageView<RGBA> image_rgba, const Point& p) {
    check(containing(image_rgba.bounds(), p));
    return image_rgba.value_at(p);
  }

  template <typename ImageView, typename P>
  bool all_of(ImageView image_view, const Rect& rect, P&& predicate) {
    check_rect(image_view, rect);
    for (auto y = 0; y < rect.h; ++y) {
      auto row = image_view.values_at(rect.x, rect.y + y);
      for (auto x = 0; x < rect.w; ++x, ++row)
        if (!predicate(*row))
          return false;
    }
    return true;
  }

  template <typename Image, typename F>
  void with_view(Image& image, F&& func) {
    switch (image.type()) {
      case ImageType::RGBA: return func(image.view<RGBA>());
      case ImageType::RGBAF: return func(image.view<RGBAF>());
      case ImageType::Mono: return func(image.view<RGBA::Channel>());
    }
  }

  template <typename ImageView, typename F>
  void for_each_pixel(ImageView image_view, F&& func) {
    std::for_each(image_view.values(), 
      image_view.values() + image_view.size(), std::forward<F>(func));
  }

  template <typename ImageView, typename F>
  void for_each_pixel(ImageView image_view, Rect rect, F&& func) {
    for (auto y = 0; y < rect.h; ++y) {
      auto row = image_view.values_at(rect.x, rect.y + y);
      for (auto x = 0; x < rect.w; ++x, ++row)
        func(*row);
    }
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

  RGBAF blend(const RGBAF& a, const RGBAF& b, float mix) {
    const auto blend = [](auto a, auto b, auto mix) {
      return a + (b - a) * mix;
    };
    // TODO: also weight by alpha
    return {
      blend(a.r, b.r, mix),
      blend(a.g, b.g, mix),
      blend(a.b, b.b, mix),
      blend(a.a, b.a, mix),
    };
  }

  RGBAF sample_linear(ImageView<const RGBAF> image, const PointF& point, 
      const RGBAF& background) {
    const auto sample = [&](const Point& p) {
      return (containing(image.bounds(), p) ? image.value_at(p) : background);
    };
    const auto int_remainder = [](real value) {
      const auto i = std::floor(value);
      return std::pair(to_int(i), static_cast<float>(value - i));
    };
    const auto [ix, rx] = int_remainder(point.x);
    const auto [iy, ry] = int_remainder(point.y);
    const auto c0 = sample(Point(ix, iy));
    const auto c1 = sample(Point(ix + 1, iy));
    const auto c2 = sample(Point(ix, iy + 1));
    const auto c3 = sample(Point(ix + 1, iy + 1));
    return blend(blend(c0, c1, rx), blend(c2, c3, rx), ry);
  }

  template<typename T, typename Sample>
  Image rotate_image_sample(ImageView<const T> source, 
      real angle, const Sample&& sample) {
    const auto cos = std::cos(angle);
    const auto sin = std::sin(angle);
    const auto transform = [&](real x, real y) -> PointF {
      return { x * cos + y * sin, y * cos - x * sin };
    };

    const auto w2 = source.width() / 2.0;
    const auto h2 = source.height() / 2.0;
    const auto p0 = transform(-w2, -h2);
    const auto p1 = transform( w2, -h2);
    const auto p2 = transform(-w2,  h2);
    const auto p3 = transform( w2,  h2);
    const auto mx = std::max(std::max(std::max(p0.x, p1.x), p2.x), p3.x);
    const auto my = std::max(std::max(std::max(p0.y, p1.y), p2.y), p3.y);

    // make destination size even/odd like source
    const auto dx = to_int(mx * 2.0 + 0.5);
    const auto dy = to_int(my * 2.0 + 0.5);
    auto dest = Image(source.type(),
      dx + (source.width() % 2 != dx % 2 ? 1 : 0), 
      dy + (source.height() % 2 != dy % 2 ? 1 : 0));
    const auto dest_view = dest.view<T>();

    const auto source_center = PointF(
      to_real(source.width()) / 2.0, 
      to_real(source.height()) / 2.0);
    const auto dest_center = PointF(
      to_real(dest.width()) / 2.0, 
      to_real(dest.height()) / 2.0);
    for (auto y = 0; y < dest.height(); ++y)
      for (auto x = 0; x < dest.width(); ++x)
        dest_view.value_at(Point(x, y)) = sample(
          transform(x - dest_center.x, y - dest_center.y) + source_center);

    return dest;
  }

  Image rotate_image_linear(ImageView<const RGBAF> image_rgbaf, real angle, const RGBAF& background_rgbaf) {
    return rotate_image_sample(image_rgbaf, angle, 
      [&](const PointF& pos) {
        return sample_linear(image_rgbaf, pos, background_rgbaf);
      });
  }

  Image rotate_image_nearest(ImageView<const RGBAF> image_rgbaf, real angle, const RGBAF& background_rgbaf) {
    return rotate_image_sample(image_rgbaf, angle, 
      [&](const PointF& pos) {
        const auto point = Point(to_int(pos.x + 0.5), to_int(pos.y + 0.5));
        return (containing(image_rgbaf.bounds(), point) ? 
          image_rgbaf.value_at(point) : background_rgbaf);
      });
  }
} // namespace

Image clone_image(const Image& image, const Rect& rect, int padding) {
  if (empty(rect))
    return clone_image(image, image.bounds(), padding);
  check_rect(image, rect);
  auto clone = Image(image.type(), rect.w + padding * 2, rect.h + padding * 2);
  if (padding)
    std::memset(clone.data().data(), 0x00, clone.data().size_bytes());
  copy_rect(image, rect, clone, padding, padding);
  return clone;
}

void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy) {
  with_view(source, [&](auto source_view) {
    using Value = std::remove_const_t<decltype(source_view)::Value>;
    const auto dest_view = dest.view<Value>();
    const auto [sx, sy, w, h] = source_rect;
    const auto dest_rect = Rect{ dx, dy, w, h };
    if (source_rect == source.bounds() &&
        dest_rect == dest.bounds() &&
        source_rect == dest_rect) {
      std::memcpy(dest_view.values(), source_view.values(),
        to_unsigned(w * h) * sizeof(Value));
    }
    else {
      check_rect(source, source_rect);
      check_rect(dest, dest_rect);
      for (auto y = 0; y < h; ++y)
        std::memcpy(
          dest_view.values_at(dx, dy + y),
          source_view.values_at(sx, sy + y),
          to_unsigned(w) * sizeof(Value));
    }
  });
}

void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy) {
  const auto source_rgba = source.view<RGBA>();
  const auto dest_rgba = dest.view<RGBA>();
  const auto [sx, sy, w, h] = source_rect;
  check_rect(source, source_rect);
  check_rect(dest, { dx, dy, h, w });
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      std::memcpy(
        dest_rgba.values_at(dx + h-1 - y, dy + x),
        source_rgba.values_at(sx + x, sy + y),
        sizeof(RGBA));
}

void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy,
    const std::vector<PointF>& mask_vertices) {
  const auto source_rgba = source.view<RGBA>();
  const auto dest_rgba = dest.view<RGBA>();
  const auto [sx, sy, w, h] = source_rect;
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      if (point_in_polygon(x + 0.5, y + 0.5, mask_vertices))
        checked_rgba_at(dest_rgba, { dx + x, dy + y }) = 
          checked_rgba_at(source_rgba, { sx + x, sy + y });
}

void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, 
    Image& dest, int dx, int dy, const std::vector<PointF>& mask_vertices) {
  const auto source_rgba = source.view<RGBA>();
  const auto dest_rgba = dest.view<RGBA>();
  const auto [sx, sy, w, h] = source_rect;
  for (auto y = 0; y < h; ++y)
    for (auto x = 0; x < w; ++x)
      if (point_in_polygon(x + 0.5, y + 0.5, mask_vertices))
        checked_rgba_at(dest_rgba, { dx + (h-1 - y), dy + x }) =
          checked_rgba_at(source_rgba, { sx + x, sy + y });
}

Image extrude_image(const Image& image, int count, WrapMode mode) {
  auto output = clone_image(image, image.bounds(), count);
  const auto rect = expand(output.bounds(), -count);

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

    with_view(output, [&](auto image_view) {
      std::memcpy(&image_view.value_at({ dx0 + 1, dy0 }), 
                  &image_view.value_at({ dx0 + 1, sy0 }),
                  to_unsigned(dx1 - dx0 - 1) * image_view.pixel_size());

      std::memcpy(&image_view.value_at({ dx0 + 1, dy1 }), 
                  &image_view.value_at({ dx0 + 1, sy1 }), 
                  to_unsigned(dx1 - dx0 - 1) * image_view.pixel_size());

      for (auto y = dy0; y <= dy1; ++y)
        image_view.value_at({ dx0, y }) = image_view.value_at({ sx0, y });

      for (auto y = dy0; y <= dy1; ++y)
        image_view.value_at({ dx1, y }) = image_view.value_at({ sx1, y });
    });
  }
  return output;
}

bool is_opaque(const Image& image, const Rect& rect) {
  if (empty(rect))
    return is_opaque(image, image.bounds());

  return all_of(image.view<RGBA>(), rect,
    [](const RGBA& rgba) { return (rgba.a == 255); });
}

bool is_fully_transparent(const Image& image, int threshold, const Rect& rect) {
  if (empty(rect))
    return is_fully_transparent(image, threshold, image.bounds());

  return all_of(image.view<RGBA>(), rect, 
    [&](const RGBA& rgba) { return (rgba.a < threshold); });
}

bool is_fully_black(const Image& image, int threshold, const Rect& rect) {
  if (empty(rect))
    return is_fully_black(image, threshold, image.bounds());

  return all_of(image.view<RGBA>(), rect,
    [&](const RGBA& rgba) { return (to_gray(rgba) < threshold); });
}

bool is_identical(const Image& image_a, const Rect& rect_a, const Image& image_b, const Rect& rect_b) {
  check_rect(image_a, rect_a);
  check_rect(image_b, rect_b);
  if (rect_a.w != rect_b.w || rect_a.h != rect_b.h)
    return false;

  const auto image_a_rgba = image_a.view<RGBA>();
  const auto image_b_rgba = image_b.view<RGBA>();
  for (auto y = 0; y < rect_a.h; ++y)
    if (std::memcmp(
        image_a_rgba.values_at(rect_a.x, rect_a.y + y),
        image_b_rgba.values_at(rect_b.x, rect_b.y + y),
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
  const auto image_rgba = image.view<RGBA>();
  for (const auto& corner : corners)
    colors.push_back(image_rgba.value_at(corner));
  std::sort(begin(colors), end(colors));
  return colors[1];
}

void replace_color(Image& image, RGBA original, RGBA color) {
  const auto image_rgba = image.view<RGBA>();
  if (original != color)
    std::replace(image_rgba.values(), 
      image_rgba.values() + image_rgba.size(), original, color);
}

std::vector<Rect> find_islands(const Image& image, int merge_distance,
    bool gray_levels, const Rect& rect) {
  if (empty(rect))
    return find_islands(image, merge_distance, gray_levels,
      get_used_bounds(image, gray_levels));

  auto levels = (gray_levels ?
    get_gray_levels(image, rect) :
    get_alpha_levels(image, rect));
  const auto levels_mono = levels.view<RGBA::Channel>();

  auto islands = std::vector<Rect>();
  for (auto y = 0; y < rect.h; ++y)
    for (auto x = 0; x < rect.w; ++x)
      if (levels_mono.value_at({ x, y })) {
        auto min_x = x;
        auto min_y = y;
        auto max_x = x;
        auto max_y = y;
        flood_fill<8>(x, y, rect.w, rect.h,
          RGBA::Channel{ },
          [&](int x, int y) -> RGBA::Channel& { 
            return levels_mono.value_at({ x, y }); 
          },
          [&](const RGBA::Channel& pixel) { return (pixel != 0); },
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

void clear_alpha(Image& image, RGBA color) {
  for_each_pixel(image.view<RGBA>(),
    [&](RGBA& rgba) {
      if (rgba.a == 0)
        rgba = color;
    });
}

void make_opaque(Image& image) {
  for_each_pixel(image.view<RGBA>(),
    [&](RGBA& rgba) {
      rgba.a = 255;
    });
}

void make_opaque(Image& image, RGBA background) {
  for_each_pixel(image.view<RGBA>(),
    [&](RGBA& rgba) {
      if (rgba.a == 0)
        rgba = background;
      else
        rgba.a = 255;
    });
}

void premultiply_alpha(Image& image) {
  const auto image_rgba = image.view<RGBA>();
  const auto multiply = [](int channel, int alpha) {
    return RGBA::to_channel(channel * alpha / 255);
  };
  const auto size = image_rgba.size();
  auto rgba = image_rgba.values();
  for (auto i = 0; i < size; ++i, ++rgba) {
    rgba->r = multiply(rgba->r, rgba->a);
    rgba->g = multiply(rgba->g, rgba->a);
    rgba->b = multiply(rgba->b, rgba->a);
  }
}

void bleed_alpha(Image& image) {
  const auto image_rgba = image.view<RGBA>();
  rjm_texbleed(reinterpret_cast<unsigned char*>(image_rgba.values()),
    image.width(), image.height(), 3, sizeof(RGBA),
    image.width() * to_int(sizeof(RGBA)));
}

Image get_alpha_levels(const Image& image, const Rect& rect) {
  if (empty(rect))
    return get_alpha_levels(image, get_used_bounds(image, false));
  check_rect(image, rect);

  auto result = Image(ImageType::Mono, rect.w, rect.h);
  const auto source_rgba = image.view<RGBA>();
  auto dest = result.view<RGBA::Channel>().values();
  for_each_pixel(source_rgba, rect, 
    [&](const RGBA& color) { *dest++ = color.a; });
  return result;
}

Image get_gray_levels(const Image& image, const Rect& rect) {
  if (empty(rect))
    return get_gray_levels(image, get_used_bounds(image, true));
  check_rect(image, rect);

  auto result = Image(ImageType::Mono, rect.w, rect.h);
  const auto source_rgba = image.view<RGBA>();
  auto dest = result.view<RGBA::Channel>().values();
  for_each_pixel(source_rgba, rect, 
    [&](const RGBA& color) { *dest++ = to_gray(color); });
  return result;
}

Image resize_image(const Image& image, real scale, ResizeFilter filter) {
  const auto width = to_int(image.width() * scale + 0.5);
  const auto height = to_int(image.height() * scale + 0.5);
  if (width == image.width() && height == image.height())
    return clone_image(image);

  if (filter == ResizeFilter::undefined &&
      std::fmod(scale, 1.0f) == 0)
    filter = ResizeFilter::box;

  auto output = Image(image.type(), width, height);
  auto data_type = stbir_datatype{ };
  auto pixel_layout = stbir_pixel_layout{ };
  switch (image.type()) {
    case ImageType::RGBA:
      pixel_layout = STBIR_RGBA;
      data_type = STBIR_TYPE_UINT8_SRGB;
      break;
    case ImageType::RGBAF:
      pixel_layout = STBIR_RGBA;
      data_type = STBIR_TYPE_FLOAT;
      break;
    case ImageType::Mono:
      pixel_layout = STBIR_1CHANNEL;
      data_type = STBIR_TYPE_UINT8;
      break;
  }
  const auto edge_mode = STBIR_EDGE_CLAMP;
  const auto bytes_per_pixel = static_cast<int>(get_pixel_size(image.type()));
  if (!stbir_resize(image.data().data(),
        image.width(), image.height(), image.width() * bytes_per_pixel,
        output.data().data(), width, height, width * bytes_per_pixel,
        pixel_layout, data_type, edge_mode, static_cast<stbir_filter>(filter)))
    throw std::runtime_error("resizing image failed");
  return output;
}

Image convert_to_linear(const Image& image, const Rect& rect) {
  if (empty(rect))
    return convert_to_linear(image, image.bounds());
  check_rect(image, rect);

  auto result = Image(ImageType::RGBAF, rect.w, rect.h);
  auto dest = result.view<RGBAF>().values();
  for_each_pixel(image.view<RGBA>(), rect, 
    [&](const RGBA& c) { *dest++ = srgb_to_linear(c); });
  return result;
}

Image convert_to_srgb(const Image& image, const Rect& rect) {
  if (empty(rect))
    return convert_to_srgb(image, image.bounds());
  check_rect(image, rect);

  auto result = Image(ImageType::RGBA, rect.w, rect.h);
  auto dest = result.view<RGBA>().values();
  for_each_pixel(image.view<RGBAF>(), rect,
    [&](const RGBAF& c) { *dest++ = linear_to_srgb(c); });
  return result;
}

Image rotate_image(const Image& image, real angle, RGBA background, RotateMethod method) {
  angle = std::fmod(std::fmod(angle, 360) + 360, 360);

  if (method == RotateMethod::undefined) {
    method = RotateMethod::linear;  
    if (std::abs(std::fmod(angle, 90)) < 0.0001)
      method = RotateMethod::nearest;
  }
  angle = deg_to_rad(angle);

  const auto image_rgbaf = image.view<const RGBAF>();
  const auto background_rgbaf = srgb_to_linear(background);
  switch (method) {
    case RotateMethod::undefined:
    case RotateMethod::nearest:
      return rotate_image_nearest(image_rgbaf, angle, background_rgbaf);

    case RotateMethod::linear:
      return rotate_image_linear(image_rgbaf, angle, background_rgbaf);
  }
  return { };
}

} // namespace
