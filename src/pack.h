#pragma once

#include <vector>
#include <optional>

struct PackSize {
  int id;
  int width;
  int height;
};

struct PackRects {
  int id;
  int x;
  int y;
  bool rotated;
};

struct PackResult {
  int width;
  int height;
  std::vector<PackRects> rects;
};

struct PackSettings {
  std::optional<int> max_rects_method;
  bool power_of_two;
  bool square;
  bool allow_rotate;
  int align_width;
  int border_padding;
  int over_allocate;
  int min_width;
  int min_height;
  int max_width;
  int max_height;
};

// ensure that settings are sane (e.g. max width > 0) and all sizes can fit.
std::vector<PackResult> pack(const PackSettings& settings, const std::vector<PackSize>& sizes);
