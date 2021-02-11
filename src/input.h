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

struct Texture {
  FilenameSequence filename;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool allow_rotate{ };
  int padding{ };
  RGBA colorkey;
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
  std::map<std::string, std::string> tags;
  bool rotated{ };
  int texture_index{ };
};

std::vector<Sprite> parse_definition(const Settings& settings);
