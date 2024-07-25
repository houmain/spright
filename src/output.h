#pragma once

#include "packing.h"

namespace spright {

struct Texture {
  const Slice* slice;
  const Output* output;
  std::filesystem::path filename;
  int map_index;
};

std::vector<Texture> get_textures(const Settings& settings,
  const std::vector<Slice>& slices);

void evaluate_expressions(const Settings& settings,
  std::vector<Sprite>& sprites,
  std::vector<Texture>& textures,
  VariantMap& variables);

void complete_description_definitions(
  const Settings& settings,
  std::vector<Description>& descriptions,
  const VariantMap& variables);

std::string dump_description(
  const std::string& template_source,
  const std::vector<Sprite>& sprites,
  const std::vector<Slice>& slices);

void output_descriptions(
  const Settings& settings,
  const std::vector<Description>& descriptions,
  const std::vector<Input>& inputs,
  const std::vector<Sprite>& sprites,
  const std::vector<Slice>& slices,
  const std::vector<Texture>& textures,
  const VariantMap& variables);

Image get_slice_image(const Slice& slice, int map_index = -1);
Animation get_slice_animation(const Slice& slice, int map_index = -1);
void output_textures(std::vector<Texture>& textures);

} // namespace
