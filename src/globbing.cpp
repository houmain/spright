
#include "FilenameSequence.h"
#include "common.h"
#include "glob/glob.h"

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
        return std::exchange(m_first_filename, { });
      m_sequence.set_count(m_count);
      m_first_filename.clear();
      return std::exchange(m_sequence, { });
    }
  };
} // namespace

bool is_globbing_pattern(const std::string& filename) {
  return (filename.find('*') != std::string::npos);
}

std::vector<FilenameSequence> glob_sequences(const std::string& pattern) {
  auto paths = glob::rglob(pattern);
  std::sort(begin(paths), end(paths));

  auto sequence_merger = SequenceMerger();
  auto sequences = std::vector<FilenameSequence>();
  for (const auto& path : paths) {
    const auto filename = path_to_utf8(path);
    if (!sequence_merger.add(filename)) {
      sequences.push_back(sequence_merger.flush());
      sequence_merger.add(filename);
    }
  }
  if (!sequence_merger.flushed())
    sequences.push_back(sequence_merger.flush());

  return sequences;
}
