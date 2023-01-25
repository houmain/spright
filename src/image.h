#pragma once

#include "common.h"
#include <filesystem>

namespace spright {

class Image {
public:
  Image() = default;
  Image(int width, int height);
  Image(int width, int height, const RGBA& background);
  Image(std::filesystem::path path, std::filesystem::path filename);
  Image(Image&& rhs);
  Image& operator=(Image&& rhs);
  ~Image();
  Image clone(const Rect& rect = {}) const;
  explicit operator bool() const { return m_data != nullptr; }

  const std::filesystem::path& path() const { return m_path; }
  const std::filesystem::path& filename() const { return m_filename; }
  int width() const { return m_width; }
  int height() const { return m_height; }
  Rect bounds() const { return { 0, 0, m_width, m_height }; }
  const RGBA* rgba() const { return m_data; }
  RGBA* rgba() { return m_data; }
  RGBA& rgba_at(const Point& p) { return m_data[p.y * m_width + p.x]; }
  const RGBA& rgba_at(const Point& p) const { return m_data[p.y * m_width + p.x]; }

private:
  std::filesystem::path m_path;
  std::filesystem::path m_filename;
  RGBA* m_data{ };
  int m_width{ };
  int m_height{ };
};

class MonoImage {
public:
  using Value = uint8_t;

  MonoImage(int width, int height);
  MonoImage(int width, int height, Value background);
  MonoImage(MonoImage&& rhs);
  MonoImage& operator=(MonoImage&& rhs);
  ~MonoImage();

  int width() const { return m_width; }
  int height() const { return m_height; }
  Rect bounds() const { return { 0, 0, m_width, m_height }; }
  const Value* data() const { return m_data; }
  Value* data() { return m_data; }
  Value& value_at(const Point& p) { return m_data[p.y * m_width + p.x]; }
  const Value& value_at(const Point& p) const { return m_data[p.y * m_width + p.x]; }

private:
  Value* m_data{ };
  int m_width{ };
  int m_height{ };
};

enum class ResizeFilter {
  Default,
  Box,
  Triangle,
  CubicSplite,
  CatmussRom,
  Mitchell
};

void save_image(const Image& image, const std::filesystem::path& filename);
Image resize_image(const Image& image, float scale, ResizeFilter filter);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, 
  int dx, int dy, const std::vector<PointF>& mask_vertices);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, 
  int dx, int dy, const std::vector<PointF>& mask_vertices);
void extrude_rect(Image& image, const Rect& rect, bool left, bool top, bool right, bool bottom);
void draw_rect(Image& image, const Rect& rect, const RGBA& color);
void draw_line(Image& image, const Point& p0, const Point& p1, 
  const RGBA& color, bool omit_last = false);
void draw_line_stipple(Image& image, const Point& p0, const Point& p1, 
  const RGBA& color, int stipple, bool omit_last = false);
void fill_rect(Image& image, const Rect& rect, const RGBA& color);
bool is_opaque(const Image& image, const Rect& rect = { });
bool is_fully_transparent(const Image& image, int threshold = 1, const Rect& rect = { });
bool is_fully_black(const Image& image, int threshold = 1, const Rect& rect = { });
bool is_identical(const Image& image_a, const Rect& rect_a, const Image& image_b, const Rect& rect_b);
Rect get_used_bounds(const Image& image, bool gray_levels, int threshold = 1, const Rect& rect = { });
RGBA guess_colorkey(const Image& image);
void replace_color(Image& image, RGBA original, RGBA color);
std::vector<Rect> find_islands(const Image& image, int merge_distance, 
  bool gray_levels, const Rect& rect = { });
void clear_alpha(Image& image);
void make_opaque(Image& image, RGBA background);
void premultiply_alpha(Image& image);
void bleed_alpha(Image& image);
MonoImage get_alpha_levels(const Image& image, const Rect& rect = { });
MonoImage get_gray_levels(const Image& image, const Rect& rect = { });

} // namespace
