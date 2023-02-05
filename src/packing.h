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

struct Texture {
  OutputPtr output;
  int index;
  int width{ };
  int height{ };
  SpriteSpan sprites;
  std::filesystem::file_time_type last_source_written_time{ };
};

std::pair<int, int> get_texture_max_size(const Output& output);
Size get_sprite_size(const Sprite& sprite);
Size get_sprite_indent(const Sprite& sprite);

std::vector<Texture> pack_sprites(std::vector<Sprite>& sprites);

void pack_binpack(const OutputPtr& output, SpriteSpan sprites,
  bool fast, std::vector<Texture>& textures);
void pack_compact(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);
void pack_single(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);
void pack_keep(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);

void recompute_texture_size(Texture& texture);

} // namespace
