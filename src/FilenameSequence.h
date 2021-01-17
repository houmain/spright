
#include <string>
#include <cassert>

// format: '{'FIRST['-'[LAST]]'}'
//   e.g.: "filename{0}.png"           -> "filename0.png" to "filename0.png"
//         "filename{10-100}.png"      -> "filename10.png" to "filename100.png"
//         "filename{00010-00100}.png" -> "filename00010.png" to "filename00100.png"
//         "filename{00010-}.png"      -> "filename00010.png", "filename00011.png", ...

class FilenameSequence {
public:
  FilenameSequence() = default;
  FilenameSequence(std::string sequence_filename)
    : m_filename(std::move(sequence_filename)) {
    parse();
    if (!is_sequence())
      set_count(1);
  }
  explicit operator bool() const { return is_sequence(); }
  const std::string& filename() const { return m_filename; }
  std::string base() const { return { begin(m_filename), begin(m_filename) + m_format_offset }; }
  std::string extension() const { return { begin(m_filename) + m_format_offset + static_cast<std::string::difference_type>(m_format_size), end(m_filename) }; }
  bool is_sequence() const { return (m_format_size != 0u); }
  bool is_infinite_sequence() const { return (is_sequence() && m_count < 0); }
  bool empty() const { return m_filename.empty(); }
  int first() const { return m_first; }
  int count() const { return m_count; }
  void set_count(int count) { m_count = count; }

  std::string get_nth_filename(int index) const {
    if (!is_sequence())
      return m_filename;

    assert(index >= 0);
    assert(m_count < 0 || index < m_count);
    return std::string(m_filename).replace(
      static_cast<std::string::size_type>(m_format_offset),
      m_format_size, to_digit_string(m_first + index));
  }


private:
  template<typename InputIt>
  bool skip_space(InputIt& it, InputIt end) {
    auto skipped = false;
    while (it != end && isspace(*it)) {
      skipped = true;
      ++it;
    }
    return skipped;
  }

  template<typename ForwardIt>
  bool skip_until(ForwardIt& it, const ForwardIt& end, const char* str) {
    while (it != end) {
      if (skip(it, end, str))
        return true;
      ++it;
    }
    return false;
  }

  template<typename ForwardIt>
  bool skip(ForwardIt& it, const ForwardIt& end, const char* str) {
    auto it2 = it;
    while (*str) {
      if (it2 == end || *str != *it2)
        return false;
      ++str;
      ++it2;
    }
    it = it2;
    return true;
  }

  template<typename InputIt>
  int read_int(InputIt& it, InputIt end) {
    auto sign = 1;
    if (it != end && *it == '-') {
      sign = -1;
      ++it;
    }
    auto value = 0;
    while (it != end && std::isdigit(*it)) {
      value = value * 10 + (*it - '0');
      ++it;
    }
    return value * sign;
  }

  void parse() {
    const auto begin = m_filename.cbegin();
    const auto end = m_filename.cend();
    auto it = begin;
    if (skip_until(it, end, "{")) {
      m_format_offset = std::distance(begin, it) - 1u;
      
      skip_space(it, end);
      auto first_begin = it;
      m_first = read_int(it, end);
      m_min_digits = static_cast<int>(std::distance(first_begin, it));

      skip_space(it, end);
      if (skip(it, end, "-")) {
        skip_space(it, end);
        if (auto last = read_int(it, end))
          m_count = std::max(last - m_first + 1, 0);
        else
          m_count = -1;
      }

      skip_space(it, end);
      if (skip(it, end, "}"))
        m_format_size = static_cast<std::string::size_type>(std::distance(begin, it) - m_format_offset);
    }
  }

  std::string to_digit_string(int value) const {
    auto string = std::to_string(value);
    auto n = m_min_digits - static_cast<int>(string.size());
    if (n > 0)
      string.insert(0, static_cast<std::string::size_type>(n), '0');
    return string;
  }

  std::string m_filename{ };
  std::string::difference_type m_format_offset{ };
  std::string::size_type m_format_size{ };
  int m_first{ };
  int m_count{ };
  int m_min_digits{ };
};

inline std::string make_sequence_filename(const std::string& base,
    const std::string& first_frame, const std::string& last_frame, 
    const std::string& extension) {
  return base + "{" + first_frame + "-" + last_frame + "}" + extension;
}
