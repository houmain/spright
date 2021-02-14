
#include "common.h"
#include <cstring>
#include <charconv>
#include <algorithm>

Rect expand(const Rect& rect, int value) {
  return { rect.x - value, rect.y - value, rect.w + value * 2, rect.h + value * 2 };
}

Rect intersect(const Rect& a, const Rect& b) {
  const auto x0 = std::max(a.x, b.x);
  const auto y0 = std::max(a.y, b.y);
  const auto x1 = std::min(a.x + a.w, b.x + b.w);
  const auto y1 = std::min(a.y + a.h, b.y + b.h);
  return { x0, y0, x1 - x0, y1 - y0 };
}

std::filesystem::path utf8_to_path(std::string_view utf8_string) {
  static_assert(sizeof(char) == sizeof(char8_t));
#if defined(__cpp_char8_t)
  return std::filesystem::path(
    reinterpret_cast<const char8_t*>(utf8_string.data()),
    reinterpret_cast<const char8_t*>(utf8_string.data() + utf8_string.size()));
#else
  return std::filesystem::u8path(utf8_string);
#endif
}

std::string path_to_utf8(const std::filesystem::path& path) {
  static_assert(sizeof(char) == sizeof(char8_t));
  const auto u8string = path.u8string();
  return std::string(
    reinterpret_cast<const char*>(u8string.data()),
    reinterpret_cast<const char*>(u8string.data() + u8string.size()));
}

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c));
}

bool is_punct(char c) {
  return std::ispunct(static_cast<unsigned char>(c));
}

char to_lower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

bool is_digit(char c) {
  return std::isdigit(static_cast<unsigned char>(c));
}

bool starts_with(std::string_view str, std::string_view with) {
  return (str.size() >= with.size() &&
    std::strncmp(str.data(), with.data(), with.size()) == 0);
}

bool ends_with(std::string_view str, std::string_view with) {
  return (str.size() >= with.size() &&
    std::strncmp(str.data() + (str.size() - with.size()),
      with.data(), with.size()) == 0);
}

std::string_view ltrim(LStringView str) {
  while (!str.empty() && is_space(str.front()))
    str = str.substr(1);
  return str;
}

std::string_view rtrim(LStringView str) {
  while (!str.empty() && is_space(str.back()))
    str = str.substr(0, str.size() - 1);
  return str;
}

std::string_view trim(LStringView str) {
  return rtrim(ltrim(str));
}

std::string_view unquote(LStringView str) {
  if (str.size() >= 2 && str.front() == str.back())
    if (str.front() == '"' || str.front() == '\'')
      return str.substr(1, str.size() - 2);
  return str;
}

void split_arguments(LStringView str, std::vector<std::string_view>* result) {
  result->clear();
  for (;;) {
    str = ltrim(str);
    if (str.empty())
      break;

    if (str.front() == '"' || str.front() == '\'') {
      auto end = str.find(str.front(), 1);
      if (end == std::string::npos)
        end = str.size();
      result->push_back(str.substr(1, end - 1));
      str = str.substr(end + 1);
    }
    else {
      auto i = 0u;
      while (i < str.size() && !is_space(str[i]))
        ++i;
      result->push_back(str.substr(0, i));
      str = str.substr(i);
    }
  }
}

std::pair<std::string_view, int> split_name_number(LStringView str) {
  auto value = 0;
  if (const auto it = std::find_if(begin(str), end(str), is_digit); it != end(str)) {
    const auto [number_end, ec] = std::from_chars(it, end(str), value);
    if (ec == std::errc{ } && number_end == end(str))
      return {
        str.substr(0, static_cast<std::string_view::size_type>(std::distance(begin(str), it))),
        value
      };
  }
  return { str, 0 };
}

int distance_to_next_multiple(int value, int divisor) {
  auto remainder = value % divisor;
  if (remainder == 0)
    return 0;
  return divisor - remainder;
}
