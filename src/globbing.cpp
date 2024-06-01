
#include "globbing.h"
#include "FilenameSequence.h"
#include "common.h"

namespace spright {

namespace {
  class SequenceMerger {
  private:
    std::string m_first_filename;
    FilenameSequence m_sequence;
    int m_count{ };

  public:
    bool add(const std::string& filename) {
      // check if it is the first file
      if (flushed()) {
        m_first_filename = filename;
        m_count = 1;
        return true;
      }
    
      // check if a sequence can be created from current and previous file
      if (!m_sequence && filename.back() != '/' && filename.back() != '\\') {
        m_sequence = try_make_sequence(m_first_filename, filename);
        if (m_sequence.count() != 2)
          return false;
        m_sequence.set_infinite();
      }
    
      // check if file is next in current sequence
      if (m_sequence && filename == m_sequence.get_nth_filename(m_count)) {
        ++m_count;
        return true;
      }
      return false;
    }

    bool flushed() const { 
      return m_first_filename.empty();
    }

    FilenameSequence flush() {
      if (m_count == 1)
        return FilenameSequence(std::exchange(m_first_filename, { }));
      m_sequence.set_count(m_count);
      m_first_filename.clear();
      return std::exchange(m_sequence, { });
    }
  };

  template<typename F>
  void for_each_file(const std::filesystem::path& path, F&& func) {
    const auto options =
      std::filesystem::directory_options::follow_directory_symlink |
      std::filesystem::directory_options::skip_permission_denied;
    auto error = std::error_code{ };
    for (const auto& file : std::filesystem::recursive_directory_iterator(path, options, error))
      if (std::filesystem::is_regular_file(file, error))
        func(file);
  }
} // namespace

bool match(std::string_view pattern, std::string_view string) {
  if (pattern == string)
    return true;

  if (pattern.empty())
    return false;

  // string empty and pattern only contains *-wildcards?
  if (string.empty())
    return (pattern.find_first_not_of("*/") == std::string::npos);

  if (pattern[0] == '?' || pattern[0] == string[0])
    return match(pattern.substr(1), string.substr(1));

  if (pattern[0] != '*')
    return false;

  const auto slash = string.find('/');

  // skip **/ in pattern or a directory in string
  if (pattern.substr(0, 3) == "**/") {
    if (match(pattern.substr(3), string))
      return true;
    if (slash != std::string::npos && match(pattern, string.substr(slash + 1)))
      return true;
    return false;
  }

  // skip * in pattern or a non-slash character in string
  if (match(pattern.substr(1), string))
    return true;
  if (slash != 0 && match(pattern, string.substr(1)))
    return true;

  return false;
}

std::vector<std::string> glob(
    const std::filesystem::path& path, const std::string& pattern) {
  auto root = (path.empty() ? "." : path) / "";
  const auto path_size = path_to_utf8(root).size();
  const auto check_supported_extension = ends_with(pattern, ".*");
  auto checked_files = size_t{ };
  for (const auto& part : utf8_to_path(pattern)) {
    if (path_to_utf8(part).find_first_of("*?") != std::string::npos) {
      auto files = std::vector<std::string>();
      for_each_file(root, [&](const std::filesystem::path& file) {
        auto file_string = path_to_utf8(file);
        file_string.erase(0, path_size);
        if (match(pattern, file_string))
          if (!check_supported_extension ||
              has_supported_extension(file_string))
            files.emplace_back(std::move(file_string));

        // abort when globbing matches less than one percent of files
        ++checked_files;
        if (checked_files % 1000 == 0)
          if (files.size() * 100 < checked_files)
            throw std::runtime_error("pattern too coarse");
      });
      return files;
    }
    root /= part;
  }
  auto error = std::error_code();
  if (std::filesystem::exists(pattern, error))
    return { pattern };
  return { };
}

bool is_globbing_pattern(std::string_view filename) {
  return (filename.find_first_of("*?") != std::string::npos);
}

std::vector<FilenameSequence> glob_sequences(
    const std::filesystem::path& path, const std::string& pattern) {
  auto paths = glob(path, pattern);
  std::sort(begin(paths), end(paths));

  auto sequence_merger = SequenceMerger();
  auto sequences = std::vector<FilenameSequence>();
  for (const auto& filename : paths) {
    if (!sequence_merger.add(filename)) {
      sequences.push_back(sequence_merger.flush());
      sequence_merger.add(filename);
    }
  }
  if (!sequence_merger.flushed())
    sequences.push_back(sequence_merger.flush());

  return sequences;
}

bool has_suffix(const std::string& filename, const std::string& suffix) {
  const auto index = filename.find(suffix);
  if (index == std::string::npos)
    return false;
  const auto extension = filename[index + suffix.length()];
  return (extension == '.' || extension == '\0');
}

std::filesystem::path add_suffix(const std::filesystem::path& filename, 
    const std::string& suffix) {
  return replace_suffix(filename, "", suffix);
}

std::filesystem::path replace_suffix(const std::filesystem::path& filename_, 
    const std::string& old_suffix, const std::string& new_suffix) {
  auto filename = path_to_utf8(filename_);
  if (!old_suffix.empty())
    if (auto it = filename.find(old_suffix); it != std::string::npos)
      return utf8_to_path(filename.replace(it, old_suffix.size(), new_suffix));
  
  auto extension = filename.find_last_of('.');
  if (extension == std::string::npos)
    extension = filename.size();
  return utf8_to_path(filename.insert(extension, new_suffix));
}

} // namespace
