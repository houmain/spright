#pragma once

#include <vector>

enum class PackMethod {
  undefined,
  MaxRects_BestShortSideFit,
  MaxRects_BestLongSideFit,
  MaxRects_BestAreaFit,
  MaxRects_BottomLeftRule,
  MaxRects_ContactPointRule
};

struct PackSize {
  int id;
  int width;
  int height;
};

struct PackRect {
  int id;
  int x;
  int y;
  bool rotated;
};

struct PackSheet {
  int width;
  int height;
  std::vector<PackRect> rects;
};

struct PackSettings {
  PackMethod method;
  int max_sheets;
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

std::vector<PackSheet> pack(PackSettings settings, std::vector<PackSize> sizes);
