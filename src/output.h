#pragma once

#include "packing.h"

namespace spright {

bool write_output_description(const Settings& settings,
  const std::vector<Sprite>& sprites, const std::vector<Texture>& textures);
Image get_output_texture(const Texture& texture, int layer_index = -1);

} // namespace
