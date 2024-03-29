
#include "packing.h"
#include "chipmunk/chipmunk.h"

namespace spright {

namespace {
  struct FreeSpace { void operator()(cpSpace* space) { cpSpaceFree(space); } };
  using SpacePtr = std::unique_ptr<cpSpace, FreeSpace>;
  struct FreeShape { void operator()(cpShape* shape) { cpShapeFree(shape); } };
  using ShapePtr = std::unique_ptr<cpShape, FreeShape>;
  struct FreeBody { void operator()(cpBody* body) { cpBodyFree(body); } };
  using BodyPtr = std::unique_ptr<cpBody, FreeBody>;

  void compact_sprites(const Slice& slice, int border_padding, int shape_padding) {
    auto space_ptr = SpacePtr(cpSpaceNew());
    const auto space = space_ptr.get();

    const auto padding = to_real(shape_padding) / 2.0;
    const auto border = to_real(border_padding) - padding;
    const auto x0 = border;
    const auto y0 = border;
    const auto x1 = to_real(slice.width) - border - 0.5;
    const auto y1 = to_real(slice.height) - border - 0.5;

    auto shapes = std::vector<ShapePtr>();
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y0), cpv(x1, y0), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y1), cpv(x1, y1), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y0), cpv(x0, y1), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x1, y0), cpv(x1, y1), 0)));

    auto bodies = std::vector<BodyPtr>();
    auto vertices = std::vector<cpVect>();
    for (const auto& sprite : slice.sprites) {
      const auto mass = 1;
      const auto moment = INFINITY;
      auto body = bodies.emplace_back(cpSpaceAddBody(space, cpBodyNew(mass, moment))).get();
      cpBodySetPosition(body, {
        to_real(sprite.trimmed_rect.x),
        to_real(sprite.trimmed_rect.y),
      });

      vertices.clear();
      std::transform(begin(sprite.vertices), end(sprite.vertices),
        std::back_inserter(vertices), [&](PointF vertex) {
          if (sprite.rotated)
            vertex = rotate_cw(vertex, sprite.trimmed_rect.h);
          return cpVect{ vertex.x, vertex.y };
        });
      shapes.emplace_back(cpSpaceAddShape(space, cpPolyShapeNew(body,
        to_int(vertices.size()), vertices.data(), cpTransformIdentity, padding)));
    }

    // TODO: improve
    for (auto i = 0; i < 1000; i++) {
      cpSpaceSetGravity(space, cpVect{ 20.0 * ((i / 100) % 2 ? 1 : -1), -100 });
      cpSpaceStep(space, 1.0 / 60);
    }

    auto i = 0u;
    for (const auto& body : bodies) {
      auto& sprite = slice.sprites[i++];
      const auto position = cpBodyGetPosition(body.get());
      sprite.trimmed_rect.x = to_int(position.x + 0.5);
      sprite.trimmed_rect.y = to_int(position.y + 0.5);
    }

    // destroy space before shapes
    space_ptr.reset();
  }
} // namespace

void pack_compact(const SheetPtr& sheet, SpriteSpan sprites,
    std::vector<Slice>& slices) {
  const auto fast = (sheet->allow_rotate == false);
  pack_binpack(sheet, sprites, slices, fast);
  for (auto& slice : slices) {
    recompute_slice_size(slice);
    compact_sprites(slice, sheet->border_padding, sheet->shape_padding);
  }
}

} // namespace
