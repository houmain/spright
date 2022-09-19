#pragma once

#include "input.h"

#if __cplusplus > 201703L && __has_include(<span>)
# include <span>
#else
# include "libs/nonstd/span.hpp"
#endif

namespace spright {

#if !defined(NONSTD_SPAN_HPP_INCLUDED)
using SpriteSpan = std::span<Sprite>;
#else
using SpriteSpan = nonstd::span<Sprite>;
#endif

struct PackedTexture {
  std::filesystem::path filename;
  int width{ };
  int height{ };
  SpriteSpan sprites;
  Alpha alpha{ };
  RGBA colorkey{ };
};

std::pair<int, int> get_texture_max_size(const Output& output);
Size get_sprite_size(const Sprite& sprite);
Size get_sprite_indent(const Sprite& sprite);

std::vector<PackedTexture> pack_sprites(std::vector<Sprite>& sprites);

void pack_binpack(const Output& output, SpriteSpan sprites,
  bool fast, std::vector<PackedTexture>& packed_textures);
void pack_compact(const Output& output, SpriteSpan sprites,
  std::vector<PackedTexture>& packed_textures);
void pack_single(const Output& output, SpriteSpan sprites,
  std::vector<PackedTexture>& packed_textures);
void pack_keep(const Output& output, SpriteSpan sprites,
  std::vector<PackedTexture>& packed_textures);

} // namespace
