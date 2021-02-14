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

enum class Trim { none, trim, crop };

enum class Alpha { keep, clear, bleed, premultiply, colorkey };

struct Texture {
  std::filesystem::path path;
  FilenameSequence filename;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  bool deduplicate{ };
  Alpha alpha{ };
  RGBA colorkey{ };
};

struct Sprite {
  std::string id;
  TexturePtr texture;
  ImagePtr source;
  Rect source_rect{ };
  Rect trimmed_source_rect{ };
  Rect rect{ };
  Rect trimmed_rect{ };
  Pivot pivot{ };
  bool integral_pivot_point{ };
  PointF pivot_point{ };
  PointF trimmed_pivot_point{ };
  Trim trim{ };
  int trim_margin{ };
  int trim_threshold{ };
  int extrude{ };
  std::map<std::string, std::string> tags;
  bool rotated{ };
  int texture_index{ };
  Size common_divisor{ };
  Point common_divisor_offset{ };
  Size common_divisor_margin{ };
};

std::vector<Sprite> parse_definition(const Settings& settings);
