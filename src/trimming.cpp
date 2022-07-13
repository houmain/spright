
#include "trimming.h"
#include "chipmunk/chipmunk.h"
extern "C" {
#include "chipmunk/cpPolyline.h"
#include "chipmunk/cpMarch.h"
}

namespace spright {

namespace {
  struct FreePolyline { void operator()(cpPolyline* line) { cpPolylineFree(line); }; };
  using PolylinePtr = std::unique_ptr<cpPolyline, FreePolyline>;

  PolylinePtr merge_polylines(const cpPolylineSet& polyline_set) {
    // TODO: find closest segments
    // rotate vertices of one to begin at segment and delete end
    // insert into other polyline at segment
#if 0
    auto count = 0;
    for (auto i = 0; i < polyline_set.count; ++i)
      count += polyline_set.lines[i]->count;

    auto merged = PolylinePtr(static_cast<cpPolyline*>(cpcalloc(1,
      sizeof(cpPolyline) + static_cast<size_t>(count) * sizeof(cpVect))));
    merged->capacity = count;
    merged->count = count;

    auto pos = merged->verts;
    for (auto i = 0; i < polyline_set.count; ++i) {
      auto line = polyline_set.lines[i];
      std::copy(line->verts, line->verts + line->count, pos);
      pos += line->count;
    }
#else
    auto line = std::add_pointer_t<cpPolyline>{ };
    for (auto i = 0; i < polyline_set.count; ++i)
      if (!line || polyline_set.lines[i]->count > line->count)
        line = polyline_set.lines[i];

    auto merged = PolylinePtr(static_cast<cpPolyline*>(cpcalloc(1,
      sizeof(cpPolyline) + static_cast<size_t>(line->count) * sizeof(cpVect))));
    merged->capacity = line->count;
    merged->count = line->count;

    std::copy(line->verts, line->verts + line->count, merged->verts);
#endif
    return merged;
  }

  PolylinePtr get_polygon_outline(const MonoImage& image, int threshold) {
    const auto sample = [](cpVect point, void *data) -> cpFloat {
      const auto& image = *static_cast<const MonoImage*>(data);
      const auto x = static_cast<int>(point.x - 0.5);
      const auto y = static_cast<int>(point.y - 0.5);
      if (x < 0 || x >= image.width() || y < 0 || y >= image.height())
        return 0;
      return image.value_at({ x, y });
    };

    const auto outlines = cpPolylineSetNew();
    cpMarchHard(
      { -1, -1, static_cast<float>(image.width() + 1), static_cast<float>(image.height() + 1) },
      static_cast<unsigned long>(image.width() + 3),
      static_cast<unsigned long>(image.height() + 3),
      static_cast<float>(threshold - 1),
      reinterpret_cast<cpMarchSegmentFunc>(cpPolylineSetCollectSegment), outlines,
      sample, const_cast<MonoImage*>(&image));

    assert(outlines->count > 0);
    const auto right = cpFloat(image.width());
    const auto bottom = cpFloat(image.height());
    for (auto i = 0; i < outlines->count; ++i) {
      auto& line = *outlines->lines[i];
      auto& verts = line.verts;
      for (auto j = 0; j < line.count; ++j) {
        verts[j].x = std::clamp(verts[j].x, cpFloat{ }, right);
        verts[j].y = std::clamp(verts[j].y, cpFloat{ }, bottom);
      }
    }

    auto outline = PolylinePtr(merge_polylines(*outlines));
    cpPolylineSetFree(outlines, true);
    return outline;
  }

  cpVect normal(const cpVect& v) {
    if (auto f = v.x * v.x + v.y * v.y; f != 0.0f) {
      f = 1.0f / std::sqrt(f);
      return { v.y * f, -v.x * f, };
    }
    return { 0, 0 };
  }

  void expand_polygon(cpPolyline& polyline, float distance) {
    if (distance == 0)
      return;
    distance /= 2;

    auto normals = std::vector<cpVect>();
    normals.reserve(static_cast<size_t>(polyline.count));
    for (auto i = 0; i < polyline.count; ++i) {
      const auto& p0 = polyline.verts[i];
      const auto& p1 = polyline.verts[(i + 1) %  polyline.count];
      normals.push_back(normal({ p1.x - p0.x, p1.y - p0.y }));
    }
    for (auto i = size_t{ }; i < normals.size(); ++i) {
      const auto& n0 = normals[i];
      const auto& n1 = normals[(i - 1 + normals.size()) % normals.size()];
      polyline.verts[i].x += (n0.x + n1.x) * distance;
      polyline.verts[i].y += (n0.y + n1.y) * distance;
    }
  }

  PolylinePtr to_convex_polygon(const cpPolyline& polyline, float tolerance) {
    return PolylinePtr(cpPolylineToConvexHull(
      const_cast<cpPolyline*>(&polyline), tolerance));
  }

  PolylinePtr simplify_polygon(const cpPolyline& polyline, float tolerance) {
    return PolylinePtr(cpPolylineSimplifyCurves(
      const_cast<cpPolyline*>(&polyline), tolerance));
  }

  std::vector<PointF> to_point_list(const cpPolyline& polyline) {
    auto vertices = std::vector<PointF>();
    vertices.reserve(static_cast<size_t>(polyline.count));
    for (auto i = 0; i < polyline.count; ++i)
      vertices.push_back({
        static_cast<float>(polyline.verts[i].x),
        static_cast<float>(polyline.verts[i].y)
      });
    return vertices;
  }
} // namespace

void trim_sprite(Sprite& sprite) {

  if (sprite.trim == Trim::none) {
    sprite.trimmed_source_rect = sprite.source_rect;
    return;
  }

  sprite.trimmed_source_rect = get_used_bounds(*sprite.source,
    sprite.trim_gray_levels, sprite.trim_threshold, sprite.source_rect);

  if (sprite.trim_margin)
    sprite.trimmed_source_rect = intersect(expand(
      sprite.trimmed_source_rect, sprite.trim_margin), sprite.source_rect);

  if (sprite.trim == Trim::convex) {
    const auto levels = (sprite.trim_gray_levels ?
      get_gray_levels(*sprite.source, sprite.trimmed_source_rect) :
      get_alpha_levels(*sprite.source, sprite.trimmed_source_rect));

    auto outline = get_polygon_outline(levels, sprite.trim_threshold);
    outline = to_convex_polygon(*outline, 0);
    outline = simplify_polygon(*outline, 3);
    expand_polygon(*outline, static_cast<float>(sprite.trim_margin));
    sprite.vertices = to_point_list(*outline);
  }
}

} // namespace
