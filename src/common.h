#pragma once

#include "Rect.h"
#include "Scheduler.h"
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <cctype>
#include <cmath>
#include <algorithm>
#include <utility>
#include <optional>

extern Scheduler scheduler;

namespace spright {

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

  uint8_t& channel(int index) { return (&r)[index]; }
  const uint8_t& channel(int index) const { return (&r)[index]; }
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

template<typename T> 
int to_int(const T& v) { 
  static_assert(std::is_floating_point_v<T> || 
    std::is_enum_v<T> || std::is_unsigned_v<T>);
  return static_cast<int>(v);
}

template<typename T>
uint8_t to_byte(const T& v) {
  static_assert(std::is_integral_v<T>);
  return static_cast<uint8_t>(v);
}

template<typename T>
unsigned int to_unsigned(const T& v) { 
  static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
  return static_cast<unsigned int>(v); 
}

template<typename T> 
real to_real(const T& v) { 
  static_assert(std::is_integral_v<T>);
  return static_cast<real>(v); 
}

std::filesystem::path utf8_to_path(std::string_view utf8_string);
std::filesystem::path utf8_to_path(const std::string& utf8_string);
std::filesystem::path utf8_to_path(const std::filesystem::path&) = delete;
std::string path_to_utf8(const std::filesystem::path& path);
std::string path_to_utf8(const std::string&) = delete;
std::filesystem::file_time_type get_last_write_time(
  const std::filesystem::path& path);
std::optional<std::filesystem::file_time_type> try_get_last_write_time(
  const std::filesystem::path& path);
bool is_digit(char c);
bool is_space(char c);
bool is_punct(char c);
char to_lower(char c);
std::string to_lower(std::string string);
std::optional<bool> to_bool(std::string_view str);
std::optional<real> to_real(std::string_view string);
std::string to_string(bool value);
std::string to_string(real value);
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
std::string read_textfile(const std::filesystem::path& filename);
void write_textfile(const std::filesystem::path& filename, std::string_view text);
bool update_textfile(const std::filesystem::path& filename, std::string_view text);
std::string remove_extension(std::string filename);
void replace_variables(std::string& expression,
  const std::function<std::string(std::string_view)>& replace_function);

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

} // namespace
