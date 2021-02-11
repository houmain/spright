#pragma once

#include "input.h"
#include <span>

struct PackedTexture {
  std::string filename;
  int width;
  int height;
  std::span<Sprite> sprites;
};

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites);
