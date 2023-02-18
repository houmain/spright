#pragma once

#include "image.h"
#include "settings.h"
#include "FilenameSequence.h"
#include <memory>
#include <map>
#include <variant>

namespace spright {

using ImagePtr = std::shared_ptr<const Image>;
using OutputPtr = std::shared_ptr<const struct Output>;
using MapVectorPtr = std::shared_ptr<const std::vector<ImagePtr>>;
using StringMap = std::map<std::string, std::string, std::less<>>;
using Variant = std::variant<bool, real, std::string>;
using VariantMap = std::map<std::string, Variant, std::less<>>;

enum class PivotX { left, center, right };
enum class PivotY { top, middle, bottom };
struct Pivot { PivotX x; PivotY y; };

enum class Trim { none, rect, convex };

enum class Alpha { keep, clear, bleed, premultiply, colorkey };

enum class Pack { binpack, rows, columns, compact, single, layers, keep };

enum class Duplicates { keep, share, drop };

struct Extrude {
  int count;
  WrapMode mode;
};

struct Scaling {
  real scale;
  ResizeFilter resize_filter;
  std::string filename_suffix;
};

struct Output {
  int index;
  std::filesystem::path input_file;
  FilenameSequence filename;
  std::string default_map_suffix;
  std::vector<std::string> map_suffixes;
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
  Duplicates duplicates{ };
  Alpha alpha{ Alpha::keep };
  RGBA colorkey{ };
  Pack pack{ Pack::binpack };
  std::vector<Scaling> scalings;
};

struct Sprite {
  int index{ };
  int input_sprite_index{ };
  std::string id;
  OutputPtr output;
  ImagePtr source;
  MapVectorPtr maps;
  Rect source_rect{ };
  Rect trimmed_source_rect{ };
  Rect rect{ };
  Rect trimmed_rect{ };
  Pivot pivot{ PivotX::center, PivotY::middle };
  PointF pivot_point{ };
  Trim trim{ Trim::none };
  int trim_margin{ };
  int trim_threshold{ };
  bool trim_gray_levels{ };
  bool crop{ };
  bool crop_pivot{ };
  Extrude extrude{ };
  StringMap tags;
  VariantMap data;
  bool rotated{ };
  int texture_output_index{ };
  Size common_divisor{ };
  Point common_divisor_offset{ };
  Size common_divisor_margin{ };
  std::vector<PointF> vertices;

  // generated
  int duplicate_of_index{ -1 };
};

struct InputDefinition {
  std::vector<Sprite> sprites;
  VariantMap variables;
};

InputDefinition parse_definition(const Settings& settings);

} // namespace
