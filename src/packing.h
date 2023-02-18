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
  int output_index{ };
  SpriteSpan sprites;
  int width{ };
  int height{ };

  // generated
  int index{ };
  std::string filename;
  std::filesystem::file_time_type last_source_written_time{ };
  bool layered{ };
};

std::pair<int, int> get_texture_max_size(const Output& output);
Size get_sprite_size(const Sprite& sprite);
Size get_sprite_indent(const Sprite& sprite);
void create_textures_from_filename_indices(const OutputPtr& output_ptr, 
    SpriteSpan sprites, std::vector<Texture>& textures);
void recompute_texture_size(Texture& texture);

std::vector<Texture> pack_sprites(std::vector<Sprite>& sprites);

void pack_binpack(const OutputPtr& output, SpriteSpan sprites,
  bool fast, std::vector<Texture>& textures);
void pack_compact(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);
void pack_single(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);
void pack_keep(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);
void pack_lines(bool horizontal, const OutputPtr& output,
  SpriteSpan sprites, std::vector<Texture>& textures);
void pack_layers(const OutputPtr& output, SpriteSpan sprites,
  std::vector<Texture>& textures);

[[noreturn]] void throw_not_all_sprites_packed();

} // namespace
