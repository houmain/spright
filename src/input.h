#pragma once

#include "image.h"
#include "settings.h"
#include "FilenameSequence.h"
#include <memory>
#include <map>

using ImagePtr = std::shared_ptr<const Image>;
using TexturePtr = std::shared_ptr<const struct Texture>;

enum class PivotX { left, center, right, custom };
enum class PivotY { top, middle, bottom, custom };
struct Pivot { PivotX x; PivotY y; };

enum class Trim { none, rect, convex };

enum class Alpha { keep, clear, bleed, premultiply, colorkey };

struct Texture {
  FilenameSequence filename;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool square{ };
  int align_width{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  bool deduplicate{ };
  Alpha alpha{ };
  RGBA colorkey{ };
};

struct Sprite {
  int index{ };
  std::string id;
  TexturePtr texture;
  ImagePtr source;
  Rect source_rect{ };
  Rect trimmed_source_rect{ };
  Rect rect{ };
  Rect trimmed_rect{ };
  Pivot pivot{ };
  PointF pivot_point{ };
  Trim trim{ };
  bool crop{ };
  int trim_margin{ };
  int trim_threshold{ };
  int extrude{ };
  std::map<std::string, std::string> tags;
  bool rotated{ };
  int texture_index{ };
  Size common_divisor{ };
  Point common_divisor_offset{ };
  Size common_divisor_margin{ };
  std::vector<PointF> vertices;
};

std::vector<Sprite> parse_definition(const Settings& settings);
