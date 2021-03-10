#pragma once

#include "FilenameSequence.h"
#include <vector>

bool match(std::string_view pattern, std::string_view string);
std::vector<std::string> glob(const std::string& pattern);

bool is_globbing_pattern(const std::string& filename);
std::vector<FilenameSequence> glob_sequences(const std::string& pattern);
