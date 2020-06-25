
#include "common.h"
#include <cstring>
#include <algorithm>

bool is_space(char c) {
  return std::isspace(static_cast<unsigned char>(c));
}

bool is_punct(char c) {
  return std::ispunct(static_cast<unsigned char>(c));
}

char to_lower(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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
