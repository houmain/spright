#pragma once

#include "input.h"
#include <span>

struct PackedTexture {
  std::filesystem::path path;
  std::filesystem::path filename;
  int width;
  int height;
  std::span<Sprite> sprites;
  Alpha alpha;
  RGBA colorkey;
};

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites);
