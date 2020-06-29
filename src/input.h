#pragma once

#include "image.h"
#include "settings.h"
#include <memory>
#include <map>

using ImagePtr = std::shared_ptr<const Image>;

enum class PivotX { left, center, right, custom };
enum class PivotY { top, middle, bottom, custom };
struct Pivot { PivotX x; PivotY y; };

enum class Trim { none, trim, crop };

struct Sprite {
  std::string id;
  ImagePtr source;
  Rect source_rect{ };
  Rect trimmed_source_rect{ };
  Rect rect{ };
  Rect trimmed_rect{ };
  Pivot pivot{ };
  bool integral_pivot_point{ };
  PointF pivot_point{ };
  PointF trimmed_pivot_point{ };
  int margin{ };
  Trim trim{ };
  std::map<std::string, std::string> tags;
};

std::vector<Sprite> parse_definition(const Settings& settings);
