#pragma once

#include "packing.h"

namespace spright {

void evaluate_expressions(const Settings& settings,
    std::vector<Sprite>& sprites,
    std::vector<Slice>& slices,
    VariantMap& variables);
std::string get_description(const std::string& template_source,
  const std::vector<Sprite>& sprites, const std::vector<Slice>& slices);
bool write_output_description(const Settings& settings,
  const std::vector<Sprite>& sprites, const std::vector<Slice>& slices,
  const VariantMap& variables);
void output_textures(const Settings& settings, std::vector<Slice>& slices);

} // namespace
