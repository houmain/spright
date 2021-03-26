#pragma once

#include "common.h"
#include <filesystem>

class Image {
public:
  Image(std::filesystem::path path, std::filesystem::path filename);
  Image(int width, int height, const RGBA& background = { });
  Image(Image&& rhs);
  Image& operator=(Image&& rhs);
  ~Image();
  Image clone(const Rect& rect = {}) const;

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

void save_image(const Image& image, const std::filesystem::path& filename);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy);
void copy_rect(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy, const std::vector<PointF>& mask_vertices);
void copy_rect_rotated_cw(const Image& source, const Rect& source_rect, Image& dest, int dx, int dy, const std::vector<PointF>& mask_vertices);
void extrude_rect(Image& image, const Rect& rect, bool left, bool top, bool right, bool bottom);
void draw_rect(Image& image, const Rect& rect, const RGBA& color);
void draw_line(Image& image, int x0, int y0, int x1, int y1, const RGBA& color);
void fill_rect(Image& image, const Rect& rect, const RGBA& color);
bool is_opaque(const Image& image, const Rect& rect = { });
bool is_fully_transparent(const Image& image, const Rect& rect = { }, int threshold = 1);
bool is_identical(const Image& image_a, const Rect& rect_a, const Image& image_b, const Rect& rect_b);
Rect get_used_bounds(const Image& image, const Rect& rect = { }, int threshold = 1);
RGBA guess_colorkey(const Image& image);
void replace_color(Image& image, RGBA original, RGBA color);
std::vector<Rect> find_islands(const Image& image, const Rect& rect = { });
void clear_alpha(Image& image);
void make_opaque(Image& image, RGBA background);
void premultiply_alpha(Image& image);
void bleed_alpha(Image& image);
MonoImage get_alpha_levels(const Image& image, const Rect& rect = { });
MonoImage get_gray_levels(const Image& image, const Rect& rect = { });
