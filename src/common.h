#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cctype>
#include <cmath>
#include <future>
#include <algorithm>
#include <utility>
#include <optional>

namespace spright {

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

  int x0() const { return x; }
  int y0() const { return y; }
  int x1() const { return x + w; }
  int y1() const { return y + h; }
  Point center() const { return { x + w / 2, y + h / 2 }; }
};

inline bool empty(const Size& size) { return (size.x == 0 || size.y == 0); }
inline bool empty(const Rect& rect) { return (rect.w == 0 || rect.h == 0); }
inline bool operator==(const Rect& a, const Rect& b) { return std::tie(a.x, a.y, a.w, a.h) == std::tie(b.x, b.y, b.w, b.h); }
inline bool operator!=(const Rect& a, const Rect& b) { return !(a == b); }
Rect expand(const Rect& rect, int value);
Rect intersect(const Rect& a, const Rect& b);
bool containing(const Rect& a, const Rect& b);
bool containing(const Rect& a, const Point& p);
bool overlapping(const Rect& a, const Rect& b);
Rect combine(const Rect& a, const Rect& b);

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

  uint8_t gray() const {
    return static_cast<uint8_t>((r * 77 + g * 151 + b * 28) >> 8);
  }
};

inline bool operator==(const RGBA& a, const RGBA& b) { return (a.rgba == b.rgba); }
inline bool operator!=(const RGBA& a, const RGBA& b) { return (a.rgba != b.rgba); }

template<typename... T>
[[noreturn]] void error(T&&... args) {
  auto ss = std::stringstream();
  (ss << ... << std::forward<T&&>(args));
  throw std::runtime_error(ss.str());
}

template<typename... T>
void check(bool condition, T&&... args) {
  if (!condition)
    error(std::forward<T>(args)...);
}

std::filesystem::path utf8_to_path(std::string_view utf8_string);
std::filesystem::path utf8_to_path(const std::string& utf8_string);
std::filesystem::path utf8_to_path(const std::filesystem::path&) = delete;
std::string path_to_utf8(const std::filesystem::path& path);
std::string path_to_utf8(const std::string&) = delete;
bool is_space(char c);
bool is_punct(char c);
char to_lower(char c);
std::optional<float> to_float(std::string_view string);
bool starts_with(std::string_view str, std::string_view with);
bool ends_with(std::string_view str, std::string_view with);
std::string_view ltrim(LStringView str);
std::string_view rtrim(LStringView str);
std::string_view trim(LStringView str);
std::string_view unquote(LStringView str);
void split_arguments(LStringView str, std::vector<std::string_view>* result);
std::pair<std::string_view, int> split_name_number(LStringView str);
void join_expressions(std::vector<std::string_view>* arguments);
void split_expression(std::string_view str, std::vector<std::string_view>* result);
PointF rotate_cw(const PointF& point, int width);
std::string read_textfile(const std::filesystem::path& filename);
void write_textfile(const std::filesystem::path& filename, std::string_view text);
void update_textfile(const std::filesystem::path& filename, std::string_view text);

inline int floor(int v, int q) { return (v / q) * q; };
inline int ceil(int v, int q) { return ((v + q - 1) / q) * q; };
inline int sqrt(int a) { return static_cast<int>(std::sqrt(a)); }
inline int div_ceil(int a, int b) { return (b > 0 ? (a + b - 1) / b : -1); }

constexpr int ceil_to_pot(int value) {
  for (auto pot = 1; ; pot <<= 1)
    if (pot >= value)
      return pot;
}
static_assert(ceil_to_pot(0) == 1);
static_assert(ceil_to_pot(1) == 1);
static_assert(ceil_to_pot(2) == 2);
static_assert(ceil_to_pot(3) == 4);
static_assert(ceil_to_pot(4) == 4);
static_assert(ceil_to_pot(5) == 8);

constexpr int floor_to_pot(int value) {
  for (auto pot = 1; ; pot <<= 1)
    if (pot > value)
      return (pot >> 1);
}
static_assert(floor_to_pot(0) == 0);
static_assert(floor_to_pot(1) == 1);
static_assert(floor_to_pot(2) == 2);
static_assert(floor_to_pot(3) == 2);
static_assert(floor_to_pot(4) == 4);
static_assert(floor_to_pot(5) == 4);
static_assert(floor_to_pot(6) == 4);
static_assert(floor_to_pot(7) == 4);
static_assert(floor_to_pot(8) == 8);

template<typename It, typename F>
void for_each_parallel(It begin, It end, F&& func) {
  if (begin == end)
    return;
  auto results = std::vector<std::future<void>>();
  std::transform(std::next(begin), end, std::back_inserter(results),
    [&func](auto& e) { return std::async(std::launch::async, func, e); });
  func(*begin);
  for (const auto& result : results)
    result.wait();
}

} // namespace
