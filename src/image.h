#pragma once

#include "Color.h"

namespace spright {

template<typename T> class ImageView;

enum class ImageType {
  RGBA,
  RGBAF,
  Mono
};

template<typename T> ImageType get_image_type() = delete;
template<> inline ImageType get_image_type<RGBA>() { return ImageType::RGBA; }
template<> inline ImageType get_image_type<RGBAF>() { return ImageType::RGBAF; }
template<> inline ImageType get_image_type<RGBA::Channel>() { return ImageType::Mono; }

inline size_t get_pixel_size(ImageType type) {
  switch (type) {
    case ImageType::RGBA:  return 4 * sizeof(uint8_t);
    case ImageType::RGBAF: return 4 * sizeof(float);
    case ImageType::Mono:  return 1 * sizeof(uint8_t);
  }
  return 0;
}

class Image {
public:
  Image() = default;
  
  Image(ImageType type, int width, int height)
    : m_type(type), m_width(width), m_height(height),
      m_data(new std::byte[size_bytes()]) {
  }

  Image(ImageType type, int width, int height, std::byte* data)
    : m_type(type), m_width(width), m_height(height), m_data(data) {
  }

  template<typename T>
  Image(int width, int height, const T& color)
    : Image(get_image_type<T>(), width, height) {
    std::fill_n(reinterpret_cast<T*>(m_data.get()), m_width * m_height, color);
  }

  Image(Image&& rhs) noexcept 
    : m_type(rhs.m_type),
      m_width(std::exchange(rhs.m_width, 0)),
      m_height(std::exchange(rhs.m_height, 0)),
      m_data(std::exchange(rhs.m_data, nullptr)) {
  }

  Image& operator=(Image&& rhs) noexcept {
    auto tmp = std::move(rhs);
    std::swap(m_type, tmp.m_type);
    std::swap(m_width, tmp.m_width);
    std::swap(m_height, tmp.m_height);
    std::swap(m_data, tmp.m_data);
    return *this;
  }

  ~Image() = default;

  explicit operator bool() const { return static_cast<bool>(m_data); }

  ImageType type() const { return m_type; }
  int width() const { return m_width; }
  int height() const { return m_height; }
  Rect bounds() const { return { 0, 0, width(), height() }; }
  span<const std::byte> data() const { return { m_data.get(), size_bytes() }; }
  span<std::byte> data() { return { m_data.get(), size_bytes() }; }
  size_t size_bytes() const { return static_cast<size_t>(m_width * m_height) * pixel_size(); }
  size_t pixel_size() const { return get_pixel_size(m_type); }
  template<typename T> auto view() const { return ImageView<const T>(this); }
  template<typename T> auto view() { return ImageView<T>(this); }
  template<typename F> void view(F&& func) const;
  template<typename F> void view(F&& func);

private:
  ImageType m_type{ };
  int m_width{ };
  int m_height{ };
  std::unique_ptr<std::byte[]> m_data;
};

template<typename T>
class ImageView {
public:
  using Value = T;
  using ImagePtr = std::conditional_t<std::is_const_v<T>, const Image*, Image*>;

  explicit ImageView(ImagePtr image)
    : m_image(image) {
    if (get_image_type<std::remove_const_t<T>>() != image->type())
      throw std::logic_error("unexpected image type");
  }

  ImageType type() const { return m_image->type(); }
  int width() const { return m_image->width(); }
  int height() const { return m_image->height(); }
  Rect bounds() const { return { 0, 0, width(), height() }; }
  Value* values() const { 
    return reinterpret_cast<Value*>(m_image->data().data()); }
  Value* values_at(int x, int y) const { return values() + (y * width() + x); }
  Value& value_at(const Point& p) const { return *values_at(p.x, p.y); }
  int size() const { return width() * height(); }
  size_t size_bytes() const { return m_image->size_bytes(); }
  size_t pixel_size() const { return m_image->pixel_size(); }

private:
  ImagePtr m_image{ };
};

enum class WrapMode { 
  clamp, mirror, repeat 
};

enum class ResizeFilter {
  undefined,
  box,          // A trapezoid w/1-pixel wide ramps, same result as box for integer scale ratios
  triangle,     // On upsampling, produces same results as bilinear texture filtering
  cubic_spline, // The cubic b-spline (aka Mitchell-Netrevalli with B=1,C=0), gaussian-esque
  catmull_rom,  // An interpolating cubic spline
  mitchell,     // Mitchell-Netrevalli filter with B=1/3, C=1/3
  point_sample, // Simple point sampling
};

enum class RotateMethod {
  undefined,
  nearest,
  linear,
};

struct Animation {
  struct Frame {
    int index;
    Image image;
    real duration;
  };
  int width;
  int height;
  std::vector<Frame> frames;
  int max_colors;
  std::optional<RGBA> color_key;
  int loop_count;
};

using Palette = std::vector<RGBA>;

template<typename F>
void Image::view(F&& func) const {
  switch (type()) {
    case ImageType::RGBA: return func(view<RGBA>());
    case ImageType::RGBAF: return func(view<RGBAF>());
    case ImageType::Mono: return func(view<RGBA::Channel>());
  }
}

template<typename F>
void Image::view(F&& func) {
  switch (type()) {
    case ImageType::RGBA: return func(view<RGBA>());
    case ImageType::RGBAF: return func(view<RGBAF>());
    case ImageType::Mono: return func(view<RGBA::Channel>());
  }
}

inline void check(bool inside) {
  if (!inside)
    throw std::logic_error("access outside image bounds");
}

template<typename ImageOrView>
void check_rect(const ImageOrView& image, const Rect& rect) {
  check(containing(image.bounds(), rect));
}

// io
Image load_image(const std::filesystem::path& filename);
void save_image(const Image& image, const std::filesystem::path& filename);
void save_animation(const Animation& animation, const std::filesystem::path& filename);

// draw
void draw_rect(Image& image, const Rect& rect, const RGBA& color);
void draw_line(Image& image, const Point& p0, const Point& p1, 
  const RGBA& color, bool omit_last = false);
void draw_line_stipple(Image& image, const Point& p0, const Point& p1, 
  const RGBA& color, int stipple, bool omit_last = false);
void draw_rect_stipple(Image& image, const Rect& rect, const RGBA& color, int stipple);
void fill_rect(Image& image, const Rect& rect, const RGBA& color);

Image clone_image(const Image& image, const Rect& rect = { }, int padding = 0);
Image resize_image(const Image& image, real scale, ResizeFilter filter);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, 
  int dx, int dy, const std::vector<PointF>& mask_vertices);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, 
  int dx, int dy, const std::vector<PointF>& mask_vertices);
void extrude_rect(Image& image, const Rect& rect, int count, WrapMode mode, 
  bool left, bool top, bool right, bool bottom);
bool is_opaque(const Image& image, const Rect& rect = { });
bool is_fully_transparent(const Image& image, int threshold = 1, const Rect& rect = { });
bool is_fully_black(const Image& image, int threshold = 1, const Rect& rect = { });
bool is_identical(const Image& image_a, const Rect& rect_a, const Image& image_b, const Rect& rect_b);
Rect get_used_bounds(const Image& image, bool gray_levels, int threshold = 1, const Rect& rect = { });
RGBA guess_colorkey(const Image& image);
void replace_color(Image& image, RGBA original, RGBA color);
std::vector<Rect> find_islands(const Image& image, int merge_distance, 
  bool gray_levels, const Rect& rect = { });
void clear_alpha(Image& image, RGBA color);
void make_opaque(Image& image);
void make_opaque(Image& image, RGBA background);
void premultiply_alpha(Image& image);
void bleed_alpha(Image& image);
Image get_alpha_levels(const Image& image, const Rect& rect = { });
Image get_gray_levels(const Image& image, const Rect& rect = { });
Image convert_to_linear(const Image& image, const Rect& rect = { });
Image convert_to_srgb(const Image& image, const Rect& rect = { });
Image rotate_image(const Image& source, real angle, RGBA background, RotateMethod method);

} // namespace
