#pragma once

#include "packing.h"

namespace spright {

struct Texture {
  const Slice* slice;
  const Output* output;
  std::string filename;
  int map_index;
};

std::vector<Texture> get_textures(const Settings& settings,
    const std::vector<Slice>& slices);
void evaluate_expressions(const Settings& settings,
    std::vector<Sprite>& sprites, 
    std::vector<Texture>& textures, 
    VariantMap& variables);

std::string get_description(const std::string& template_source,
  const std::vector<Sprite>& sprites, 
  const std::vector<Slice>& slices,
  const std::vector<Texture>& textures);
bool output_description(const Settings& settings,
  const std::vector<Sprite>& sprites, 
  const std::vector<Slice>& slices,
  const std::vector<Texture>& textures, 
  const VariantMap& variables);

Image get_slice_image(const Slice& slice, int map_index = -1);
void output_textures(const Settings& settings,
    std::vector<Texture>& textures);

} // namespace
