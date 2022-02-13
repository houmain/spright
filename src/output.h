#pragma once

#include "packing.h"

namespace spright {

std::string get_description(const std::string& template_source,
  const std::vector<Sprite>& sprites, const std::vector<PackedTexture>& textures);
void write_output_description(const Settings& settings,
  const std::vector<Sprite>& sprites, const std::vector<PackedTexture>& textures);
Image get_output_texture(const Settings& settings, const PackedTexture& texture);

} // namespace
