#pragma once

#include "input.h"

namespace spright {

void transform_sprites(std::vector<Sprite>& sprites);
void restore_untransformed_sources(std::vector<Sprite>& sprites);
Image transform_output(Image&& source, 
  const std::vector<TransformPtr>& transforms);
real get_transform_scale(const std::vector<TransformPtr>& transforms);

} // namespace
