
#include "common.h"
#include <cstring>
#include <charconv>
#include <algorithm>
#include <fstream>
#include <iostream>
#include "cpp-base64/base64.h"

Scheduler scheduler;

namespace spright {

namespace {
  const int max_warnings = 20;
  int g_warning_count = 0;

  class WarningDeduplicator {
  public:
    ~WarningDeduplicator() {
      do_flush();
    }

    bool add(std::string_view message, int line_number) {
      auto lock = std::lock_guard(m_mutex);
      if (message == m_message && line_number == m_line_number) {
        ++m_count;
        return false;
      }
      do_flush();
      m_message = message;
      m_line_number = line_number;
      m_count = 1;
      return true;
    }

    void flush() {
      auto lock = std::lock_guard(m_mutex);
      do_flush();
    }

  private:
    void do_flush() {
      if (m_count) {
        std::cerr << m_message;
        if (m_count > 1)
          std::cerr << " (" << m_count << "x)";
        std::cerr << " in line " << m_line_number << "\n";
        m_message.clear();
        m_line_number = 0;
        m_count = 0;
      }
    }

    std::mutex m_mutex;
    std::string m_message;
    int m_line_number{ };
    int m_count{ };
  };

  WarningDeduplicator g_warning_deduplicator;
} // namespace

void warning(std::string_view message, int line_number) {
  if (g_warning_count < max_warnings)
    if (g_warning_deduplicator.add(message, line_number))
      ++g_warning_count;
}

bool has_warnings() {
  g_warning_deduplicator.flush();
  return std::exchange(g_warning_count, 0) > 0;
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

std::optional<std::filesystem::file_time_type> try_get_last_write_time(
    const std::filesystem::path& path) {
  auto error = std::error_code{ };
  auto result = std::filesystem::last_write_time(path, error);
  return (error ? std::nullopt : std::make_optional(result));
}

std::filesystem::file_time_type get_last_write_time(
    const std::filesystem::path& path) {
  auto error = std::error_code{ };
  return std::filesystem::last_write_time(path, error);
}

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c));
}

bool is_space_or_comma(char c) {
  return is_space(c) || c == ',';
}

bool is_punct(char c) {
  return std::ispunct(static_cast<unsigned char>(c));
}

char to_lower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

std::string to_lower(std::string string) {
  for (auto& c : string)
    c = to_lower(c);
  return string;
}

int equal_case_insensitive(std::string_view a, std::string_view b) {
  return std::equal(a.begin(), a.end(), b.begin(), b.end(),
    [](char a, char b) { return to_lower(a) == to_lower(b); });
}

bool is_alpha(char c) {
  return std::isalpha(static_cast<unsigned char>(c));
}

bool is_digit(char c) {
  return std::isdigit(static_cast<unsigned char>(c));
}

std::optional<bool> to_bool(std::string_view str) {
  if (str == "true") return true;
  if (str == "false") return false;
  return std::nullopt;
}

std::optional<int> to_int(std::string_view str) {
#if !defined(__GNUC__) || __GNUC__ >= 11
  auto result = int{ };
  if (std::from_chars(str.data(), 
        str.data() + str.size(), result).ec == std::errc())
    return result;
#else
  try {
    return std::stoi(std::string(str));
  }
  catch (...) {
  }
#endif
  return { };
}

std::optional<real> to_real(std::string_view str) {
#if !defined(__GNUC__) || __GNUC__ >= 11
  auto result = real{ };
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

std::string to_string(bool value) {
  return (value ? "true" : "false");
}

std::string to_string(real value) {
  const auto max_size = 20;
  auto string = std::string(max_size, ' ');
  const auto result = std::snprintf(string.data(), max_size + 1, "%.5f", value);
  if (result < 0 || result > max_size)
    throw std::runtime_error("formatting number failed");
  string.resize(static_cast<size_t>(result));
  // remove zeroes after the dot but keep one digit
  if (auto dot = string.find('.'); dot != std::string::npos)
    while (string.back() == '0' &&
           string.size() > dot + 2)
      string.pop_back();
  return string;
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
  // ([^\s]+)\s+([^\s,]+)+
  result->clear();
  str = ltrim(str);
  while (!str.empty()) {
    if (str.front() == '"' || str.front() == '\'') {
      auto end = str.find(str.front(), 1);
      if (end == std::string::npos)
        throw std::runtime_error("closing quote missing");
      result->push_back(str.substr(1, end - 1));
      str = str.substr(end + 1);
    }
    else {
      auto i = 0u;
      while (i < str.size() && !is_space_or_comma(str[i]))
        ++i;
      result->push_back(str.substr(0, i));
      str = str.substr(i);
    }

    str = ltrim(str);
    if (!str.empty() && str.front() == ',') {
      str.remove_prefix(1);
      str = ltrim(str);

      // do not allow comma after first part
      if (result->size() == 1)
        throw std::runtime_error("unexpected comma");
      
      // do not allow to end with comma
      if (str.empty())
        throw std::runtime_error("unexpected comma");
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

std::string read_textfile(const std::filesystem::path& filename) {
  auto file = std::ifstream(filename, std::ios::in | std::ios::binary);
  if (!file.good())
    throw std::runtime_error("reading file '" + path_to_utf8(filename) + "' failed");
  return std::string(std::istreambuf_iterator<char>{ file }, { });
}

std::string base64_encode_file(const std::filesystem::path& filename) {
  return base64_encode(read_textfile(filename), false);
}

void write_textfile(const std::filesystem::path& filename, std::string_view text) {
  auto error = std::error_code{ };
  std::filesystem::create_directories(filename.parent_path(), error);
  auto file = std::ofstream(filename, std::ios::out | std::ios::binary);
  if (!file.good())
    throw std::runtime_error("writing file '" + path_to_utf8(filename) + "' failed");
  file.write(text.data(), static_cast<std::streamsize>(text.size()));
}

bool update_textfile(const std::filesystem::path& filename, std::string_view text) {
  auto error = std::error_code{ };
  if (std::filesystem::exists(filename, error))
    if (const auto current = read_textfile(filename); current == text)
      return false;
  write_textfile(filename, text);
  return true;
}

std::string_view get_extension(LStringView filename) {
 const auto dot = filename.rfind('.');
  if (dot != std::string::npos)
    return filename.substr(dot);
  return "";
}

std::string remove_extension(std::string filename) {
  const auto dot = filename.rfind('.');
  if (dot != std::string::npos)
    filename.resize(dot);
  return filename;
}

bool has_supported_extension(std::string_view filename) {
  const auto ext = get_extension(filename);
  for (const auto supported : { ".png", ".gif", ".bmp", ".tga" })
    if (equal_case_insensitive(ext, supported))
      return true;
  return false;
}

void replace_variables(std::string& expression,
    const std::function<std::string(std::string_view)>& replace_function) {
  for (;;) {
    const auto begin = expression.find("{{");
    if (begin == std::string::npos)
      break;
    const auto end = expression.find("}}", begin);
    if (end == std::string::npos)
      break;
    const auto length = end - begin;
    const auto variable = trim(std::string_view{
        expression.data() + begin + 2, length - 2 });
    expression.replace(begin, length + 2, replace_function(variable));
  }
}

std::string make_identifier(std::string string) {
  for (auto& c : string)
    if (!is_alpha(c) && !is_digit(c))
      c = '_';
  
  // remove duplicate underscores and also from front and back
  auto after_underscore = true;
  for (auto i = 0u; i < string.size(); ) {
    if (string[i] == '_') {
      if (after_underscore) {
        string.erase(i, 1);
        continue;
      }
      after_underscore = true;
    }
    else {
      after_underscore = false;
    }
    ++i;
  }
  if (string.size() > 1 && string.back() == '_')
    string.pop_back();

  return string;
}

} // namespace
