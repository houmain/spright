#pragma once

#include "input.h"
#include <span>

struct PackedTexture {
  std::filesystem::path filename;
  int width{ };
  int height{ };
  std::span<Sprite> sprites;
  Alpha alpha{ };
  RGBA colorkey{ };
  int border_padding{ };
  int shape_padding{ };
};

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites);
