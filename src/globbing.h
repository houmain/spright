#pragma once

#include "FilenameSequence.h"
#include <filesystem>
#include <vector>

namespace spright {

bool match(std::string_view pattern, std::string_view string);
std::vector<std::string> glob(
  const std::filesystem::path& path, const std::string& pattern);

bool is_globbing_pattern(const std::string& filename);
std::vector<FilenameSequence> glob_sequences(
  const std::filesystem::path& path, const std::string& pattern);

} // namespace
