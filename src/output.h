#pragma once

#include "packing.h"

void output_texture(const Settings& settings, const PackedTexture& texture);
void output_definition(const Settings& settings,
  const std::vector<Sprite>& sprites, const std::vector<PackedTexture>& textures);
