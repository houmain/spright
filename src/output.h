#pragma once

#include "packing.h"

namespace spright {

void evaluate_expressions(const Settings& settings,
    std::vector<Sprite>& sprites,
    std::vector<Texture>& textures,
    VariantMap& variables);
std::string get_description(const std::string& template_source,
  const std::vector<Sprite>& sprites, const std::vector<Texture>& textures);
bool write_output_description(const Settings& settings,
  const std::vector<Sprite>& sprites, const std::vector<Texture>& textures,
  const VariantMap& variables);
Image get_output_texture(const Texture& texture, int layer_index = -1);

} // namespace
