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

std::pair<int, int> get_texture_max_size(const Texture& texture);
Size get_sprite_size(const Sprite& sprite);
Size get_sprite_indent(const Sprite& sprite);

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites);

void pack_binpack(const Texture& texture, std::span<Sprite> sprites,
  bool fast, std::vector<PackedTexture>& packed_textures);
void pack_compact(const Texture& texture, std::span<Sprite> sprites,
  std::vector<PackedTexture>& packed_textures);
