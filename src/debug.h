#pragma once

#include "packing.h"

namespace spright {

void draw_debug_info(Image& image, const Sprite& sprite, real scale = 1);
void draw_debug_info(Image& image, const Slice& slice, real scale = 1);

} // namespace
