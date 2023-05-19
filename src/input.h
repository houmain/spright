#pragma once

#include "image.h"
#include "settings.h"
#include "FilenameSequence.h"
#include <memory>
#include <map>
#include <variant>

namespace spright {

using ImagePtr = std::shared_ptr<const Image>;
using SheetPtr = std::shared_ptr<const struct Sheet>;
using OutputPtr = std::shared_ptr<const struct Output>;
using MapVectorPtr = std::shared_ptr<const std::vector<ImagePtr>>;
using StringMap = std::map<std::string, std::string, std::less<>>;
using Variant = std::variant<bool, real, std::string>;
using VariantMap = std::map<std::string, Variant, std::less<>>;

enum class AnchorX { left, center, right };
enum class AnchorY { top, middle, bottom };

template<typename T>
struct AnchorT : PointT<T> {
  AnchorX anchor_x;
  AnchorY anchor_y;
};
using Anchor = AnchorT<int>;
using AnchorF = AnchorT<real>;

enum class Trim { none, rect, convex };

enum class Alpha { keep, clear, bleed, premultiply, colorkey };

enum class Pack { binpack, rows, columns, compact, single, layers, keep };

enum class Duplicates { keep, share, drop };

struct Extrude {
  int count;
  WrapMode mode;
};

struct Input {
  int index;
  std::string source_filenames;
  std::vector<ImagePtr> sources;
};

struct Output {
  int warning_line_number{ };
  FilenameSequence filename;
  std::string default_map_suffix;
  std::vector<std::string> map_suffixes;
  Alpha alpha{ };
  RGBA colorkey{ };
  real scale{ };
  ResizeFilter scale_filter{ };
  bool debug{ };
};

struct Sheet {
  int index;
  std::string id;
  std::filesystem::path input_file;
  std::vector<OutputPtr> outputs;
  int width{ };
  int height{ };
  int max_width{ };
  int max_height{ };
  bool power_of_two{ };
  bool square{ };
  int divisible_width{ };
  bool allow_rotate{ };
  int border_padding{ };
  int shape_padding{ };
  Duplicates duplicates{ };
  Pack pack{ };
};

struct Sprite {
  int warning_line_number{ };
  int index{ };
  int input_index{ };
  int input_sprite_index{ };
  std::string id;
  SheetPtr sheet;
  ImagePtr source;
  MapVectorPtr maps;
  // the logical rect on the source
  Rect source_rect{ };
  // the actual pixels on the sources
  Rect trimmed_source_rect{ };
  AnchorF pivot{ };
  Trim trim{ Trim::none };
  int trim_margin{ };
  int trim_threshold{ };
  bool trim_gray_levels{ };
  bool crop{ };
  bool crop_pivot{ };
  Extrude extrude{ };
  Size min_bounds{ };
  Size divisible_bounds{ };
  std::string common_bounds;
  // the offset of the trimmed rect within the sprite's bounds
  Anchor align{ };
  std::string align_pivot;
  StringMap tags;
  VariantMap data;

  int slice_index{ -1 };
  // the logical rect on the output
  Rect rect{ };
  // the actual pixels on the output. same size as the trimmed source
  Rect trimmed_rect{ };
  bool rotated{ };
  // total space it allocates on the output
  Size bounds{ };
  std::vector<PointF> vertices;
  int duplicate_of_index{ -1 };
};

struct InputDefinition {
  std::vector<Input> inputs;
  std::vector<Sprite> sprites;
  VariantMap variables;
};

InputDefinition parse_definition(const Settings& settings);
int get_max_slice_count(const Sheet& sheet);

} // namespace
