#pragma once

#include "FilenameSequence.h"
#include <vector>

bool is_globbing_pattern(const std::string& filename);
std::vector<FilenameSequence> glob_sequences(const std::string& pattern);
