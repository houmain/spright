
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

  void compact_sprites(const Texture& texture, int border_padding, int shape_padding) {
    const auto space_ptr = SpacePtr(cpSpaceNew());
    const auto space = space_ptr.get();

    const auto padding = static_cast<cpFloat>(shape_padding) / 2.0;
    const auto border = static_cast<cpFloat>(border_padding) - padding;
    const auto x0 = static_cast<cpFloat>(border);
    const auto y0 = static_cast<cpFloat>(border);
    const auto x1 = static_cast<cpFloat>(texture.width) - border - 0.5;
    const auto y1 = static_cast<cpFloat>(texture.height) - border - 0.5;

    auto shapes = std::vector<ShapePtr>();
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y0), cpv(x1, y0), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y1), cpv(x1, y1), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x0, y0), cpv(x0, y1), 0)));
    shapes.emplace_back(cpSpaceAddShape(space, cpSegmentShapeNew(cpSpaceGetStaticBody(space), cpv(x1, y0), cpv(x1, y1), 0)));

    auto bodies = std::vector<BodyPtr>();
    auto vertices = std::vector<cpVect>();
    for (const auto& sprite : texture.sprites) {
      if (sprite.vertices.empty())
        continue;

      vertices.clear();
      std::transform(begin(sprite.vertices), end(sprite.vertices),
        std::back_inserter(vertices), [&](const PointF& a) { return cpVect{ a.x, a.y }; });

      const auto mass = 1;
      const auto moment = INFINITY;
      auto body = bodies.emplace_back(cpSpaceAddBody(space, cpBodyNew(mass, moment))).get();
      cpBodySetPosition(body, {
        static_cast<float>(sprite.trimmed_rect.x),
        static_cast<float>(sprite.trimmed_rect.y),
      });

      shapes.emplace_back(cpSpaceAddShape(space, cpPolyShapeNew(body,
        static_cast<int>(vertices.size()), vertices.data(), cpTransformIdentity, padding)));
    }

    // TODO: improve
    for (auto i = 0; i < 1000; i++) {
      cpSpaceSetGravity(space, cpVect{ 20.0 * ((i / 100) % 2 ? 1 : -1), -100 });
      cpSpaceStep(space, 1.0 / 60);
    }

    auto i = 0u;
    for (const auto& body : bodies) {
      auto& sprite = texture.sprites[i++];
      const auto position = cpBodyGetPosition(body.get());
      const auto dx = static_cast<int>(position.x + 0.5) - sprite.trimmed_rect.x;
      const auto dy = static_cast<int>(position.y + 0.5) - sprite.trimmed_rect.y;
      sprite.trimmed_rect.x += dx;
      sprite.trimmed_rect.y += dy;
      sprite.rect.x += dx;
      sprite.rect.y += dy;
    }
  }
} // namespace

void pack_compact(const OutputPtr& output, SpriteSpan sprites,
    std::vector<Texture>& textures) {
  pack_binpack(output, sprites, true, textures);
  for (auto& texture : textures) {
    compact_sprites(texture, output->border_padding, output->shape_padding);

    auto max_y = 0;
    for (const auto& sprite : texture.sprites)
      max_y = std::max(max_y, sprite.trimmed_rect.y + sprite.trimmed_rect.h);
    texture.height = max_y + output->border_padding;
  }
}

} // namespace
