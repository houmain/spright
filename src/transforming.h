#pragma once

#include "input.h"

namespace spright {

void transform_sprites(std::vector<Sprite>& sprites);
void restore_untransformed_sources(std::vector<Sprite>& sprites);

} // namespace
