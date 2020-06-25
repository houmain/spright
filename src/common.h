#pragma once

#include <string>
#include <vector>
#include <cctype>

struct Size {
  int x;
  int y;
};

struct Point {
  int x;
  int y;
};

struct PointF {
  float x;
  float y;
};

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

inline bool empty(const Size& size) { return (size.x == 0 && size.y == 0); }
inline bool empty(const Rect& rect) { return (rect.w == 0 && rect.h == 0); }

struct LStringView : std::string_view {
  LStringView(std::string_view s) : std::string_view(s) { }
  LStringView(const char* s) : std::string_view(s) { }
  LStringView(const std::string& s) : std::string_view(s) { }
  LStringView(std::string&& s) = delete;
};

union RGBA {
  struct {
    uint8_t r, g, b, a;
  };
  uint32_t rgba;
};

inline bool operator==(const RGBA& a, const RGBA& b) { return (a.rgba == b.rgba); }
inline bool operator!=(const RGBA& a, const RGBA& b) { return (a.rgba != b.rgba); }

bool is_space(char c);
bool is_punct(char c);
char to_lower(char c);
bool starts_with(std::string_view str, std::string_view with);
bool ends_with(std::string_view str, std::string_view with);
std::string_view ltrim(LStringView str);
std::string_view rtrim(LStringView str);
std::string_view trim(LStringView str);
std::string_view unquote(LStringView str);
void split_arguments(LStringView str, std::vector<std::string_view>* result);
