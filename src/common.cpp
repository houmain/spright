
#include "common.h"
#include <cstring>
#include <charconv>
#include <algorithm>

namespace spright {

Rect expand(const Rect& rect, int value) {
  return { rect.x - value, rect.y - value, rect.w + value * 2, rect.h + value * 2 };
}

Rect intersect(const Rect& a, const Rect& b) {
  const auto x0 = std::max(a.x0(), b.x0());
  const auto y0 = std::max(a.y0(), b.y0());
  const auto x1 = std::min(a.x1(), b.x1());
  const auto y1 = std::min(a.y1(), b.y1());
  return { x0, y0, std::max(x1 - x0, 0), std::max(y1 - y0, 0) };
}

bool containing(const Rect& a, const Rect& b) {
  return (a.x <= b.x &&
          a.y <= b.y &&
          a.x + a.w >= b.x + b.w &&
          a.y + a.h >= b.y + b.h);
}

bool containing(const Rect& a, const Point& p) {
  return (a.x <= p.x &&
          a.y <= p.y &&
          a.x + a.w > p.x &&
          a.y + a.h > p.y);
}

bool overlapping(const Rect& a, const Rect& b) {
  return !(a.x + a.w <= b.x ||
           b.x + b.w <= a.x ||
           a.y + a.h <= b.y ||
           b.y + b.h <= a.y);
}

Rect combine(const Rect& a, const Rect& b) {
  const auto x0 = std::min(a.x0(), b.x0());
  const auto y0 = std::min(a.y0(), b.y0());
  const auto x1 = std::max(a.x1(), b.x1());
  const auto y1 = std::max(a.y1(), b.y1());
  return { x0, y0, x1 - x0, y1 - y0 };
}

std::filesystem::path utf8_to_path(std::string_view utf8_string) {
#if defined(__cpp_char8_t)
  static_assert(sizeof(char) == sizeof(char8_t));
  return std::filesystem::path(
    reinterpret_cast<const char8_t*>(utf8_string.data()),
    reinterpret_cast<const char8_t*>(utf8_string.data() + utf8_string.size()));
#else
  return std::filesystem::u8path(utf8_string);
#endif
}

std::filesystem::path utf8_to_path(const std::string& utf8_string) {
  return utf8_to_path(std::string_view(utf8_string));
}

std::string path_to_utf8(const std::filesystem::path& path) {
#if defined(__cpp_char8_t)
  static_assert(sizeof(char) == sizeof(char8_t));
#endif
  const auto u8string = path.generic_u8string();
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

std::optional<float> to_float(std::string_view str) {
#if !defined(__GNUC__) || __GNUC__ >= 11
  auto result = 0.0f;
  if (std::from_chars(str.data(), 
        str.data() + str.size(), result).ec == std::errc())
    return result;
#else
  try {
    return std::stof(std::string(str));
  }
  catch (...) {
  }
#endif
  return { };
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

bool starts_with_any(std::string_view str, std::string_view with) {
  return str.find_first_of(with) == 0;
}

bool ends_with_any(std::string_view str, std::string_view with) {
  return str.find_last_of(with) == str.size() - 1;
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
    const auto sbegin = &*it;
    const auto send = sbegin + std::distance(it, end(str));
    const auto [number_end, ec] = std::from_chars(sbegin, send, value);
    if (ec == std::errc{ } && number_end == send)
      return {
        str.substr(0, static_cast<std::string_view::size_type>(std::distance(begin(str), it))),
        value
      };
  }
  return { str, 0 };
}

void join_expressions(std::vector<std::string_view>* arguments) {
  auto& args = *arguments;
  for (auto i = 0u; i + 1 < args.size(); ) {
    if (ends_with_any(args[i], "+-") ||
        starts_with_any(args[i + 1], "+-")) {
      args[i] = {
        args[i].data(),
        static_cast<std::string_view::size_type>(
          std::distance(args[i].data(), args[i + 1].data())) +
            args[i + 1].size()
      };
      args.erase(args.begin() + i + 1);
    }
    else {
      ++i;
    }
  }
}

void split_expression(std::string_view str, std::vector<std::string_view>* result) {
  result->clear();
  for (;;) {
    auto i = 0u;
    while (i < str.size() && std::string_view("+-").find(str[i]) == std::string::npos)
      ++i;
    result->push_back(trim(str.substr(0, i)));
    str = str.substr(i);

    if (str.empty())
      break;

    result->emplace_back(str.data(), 1);
    str = str.substr(1);
  }
}

PointF rotate_cw(const PointF& point, int width) {
  return { static_cast<float>(width) - point.y, point.x };
}

} // namespace
