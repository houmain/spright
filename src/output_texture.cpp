
#include "output.h"
#include "globbing.h"
#include "debug.h"

namespace spright {

namespace {
  const Image* get_source(const Sprite& sprite, int map_index) {
    if (map_index < 0)
      return sprite.source.get();
    const auto index = to_unsigned(map_index);
    if (sprite.maps && index < sprite.maps->size())
      return sprite.maps->at(index).get();
    return nullptr;
  }

  bool has_rect_vertices(const Sprite& sprite) {
    const auto& v = sprite.vertices;
    const auto [w, h] = sprite.trimmed_rect.size();
    return (v.size() == 4 &&
      v[0] == PointF(0, 0) &&
      v[1] == PointF(w, 0) &&
      v[2] == PointF(w, h) &&
      v[3] == PointF(0, h));
  }

  bool copy_sprite(Image& target, const Sprite& sprite, int map_index) try {
    const auto source = get_source(sprite, map_index);
    if (!source)
      return false;

    if (sprite.rotated) {
      if (has_rect_vertices(sprite)) {
        copy_rect_rotated_cw(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
      }
      else {
        copy_rect_rotated_cw(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y, sprite.vertices);
      }
    }
    else {
      if (has_rect_vertices(sprite)) {
        copy_rect(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y);
      }
      else {
        copy_rect(*source, sprite.trimmed_source_rect,
          target, sprite.trimmed_rect.x, sprite.trimmed_rect.y, sprite.vertices);
      }
    }

    if (sprite.extrude.count) {
      const auto left = (sprite.source_rect.x0() == sprite.trimmed_source_rect.x0());
      const auto top = (sprite.source_rect.y0() == sprite.trimmed_source_rect.y0());
      const auto right = (sprite.source_rect.x1() == sprite.trimmed_source_rect.x1());
      const auto bottom = (sprite.source_rect.y1() == sprite.trimmed_source_rect.y1());
      if (left || top || right || bottom) {
        auto rect = sprite.trimmed_rect;
        if (sprite.rotated)
          std::swap(rect.w, rect.h);
        extrude_rect(target, rect, 
          sprite.extrude.count, sprite.extrude.mode, 
          left, top, right, bottom);
      }
    }
    return true;
  }
  catch (const std::exception& ex) {
#if defined(NDEBUG)
    throw;
#else
    std::fprintf(stderr, "copying sprite '%s' failed: %s\n", 
      sprite.id.c_str(), ex.what());
    return false;
#endif
  }

  void process_alpha(Image& target, const Output& output) {
    switch (output.alpha) {
      case Alpha::keep:
        break;

      case Alpha::clear:
        clear_alpha(target);
        break;

      case Alpha::bleed:
        bleed_alpha(target);
        break;

      case Alpha::premultiply:
        premultiply_alpha(target);
        break;

      case Alpha::colorkey:
        make_opaque(target, output.colorkey);
        break;
    }
  }

  bool is_map(const Texture& texture) {
    return (texture.map_index >= 0);
  }

  bool is_up_to_date(const Texture& texture) {
    // exists and is newer than input
    return (texture.slice->last_source_written_time && 
        try_get_last_write_time(texture.filename) > 
        texture.slice->last_source_written_time);
  }

  void process_texture_image(const Texture& texture, Image& image) {
    const auto& output = *texture.output;
    process_alpha(image, output);

    if (output.scale != 1.0)
      image = resize_image(image, output.scale, output.scale_filter);
  }

  bool output_image(const Settings& settings, const Texture& texture) {
    // do not return before check if there is a map for slice
    if (!is_map(texture) && is_up_to_date(texture))
      return true;

    auto image = get_slice_image(*texture.slice, texture.map_index);
    if (!image)
      return false;

    if (is_map(texture) && is_up_to_date(texture))
      return true;
    
    process_texture_image(texture, image);

    if (settings.debug)
      draw_debug_info(image, *texture.slice, texture.output->scale);

    save_image(image, texture.filename);
    return true;
  }

  bool output_animation(const Settings& settings, const Texture& texture) {
    // do not return before check if there is a map for slice
    if (!is_map(texture) && is_up_to_date(texture))
      return true;

    auto animation = get_slice_animation(*texture.slice,
      texture.map_index);
    if (animation.frames.empty())
      return false;

    if (is_map(texture) && is_up_to_date(texture))
      return true;
    
    scheduler.for_each_parallel(animation.frames, 
      [&](Animation::Frame& frame) {
        process_texture_image(texture, frame.image);

        if (settings.debug)
          draw_debug_info(frame.image, 
            texture.slice->sprites[frame.index], texture.output->scale);
      });

    if (texture.output->alpha == Alpha::colorkey)
      animation.color_key = texture.output->colorkey;
    save_animation(animation, texture.filename);
    return true;
  }

  bool output_texture(const Settings& settings, const Texture& texture) {
    if (!texture.slice->layered)
      return output_image(settings, texture);
    return output_animation(settings, texture);
  }
} // namespace

Image get_slice_image(const Slice& slice, int map_index) {
  auto target = Image(slice.width, slice.height, RGBA{ });

  auto copied_sprite = false;
  for (const auto& sprite : slice.sprites)
    copied_sprite |= copy_sprite(target, sprite, map_index);
  if (!copied_sprite)
    return { };

  return target;
}

Animation get_slice_animation(const Slice& slice, int map_index) {
  auto animation = Animation();
  for (const auto& sprite : slice.sprites)
    if (const auto source = get_source(sprite, map_index)) {
      auto& frame = animation.frames.emplace_back();
      frame.index = static_cast<int>(animation.frames.size() - 1);
      frame.image = Image(slice.width, slice.height, RGBA());
      copy_sprite(frame.image, sprite, map_index);
      frame.duration = 0.1;
    }
  return animation;
}

std::vector<Texture> get_textures(const Settings& settings,
    const std::vector<Slice>& slices) {
  auto textures = std::vector<Texture>();
  for (const auto& slice : slices)
    for (const auto& output : slice.sheet->outputs) {
      const auto filename = settings.output_path / utf8_to_path(
        output->filename.get_nth_filename(slice.sheet_index));
      textures.push_back({ &slice, output.get(), path_to_utf8(filename), -1 });

      auto i = 0;
      for (const auto& map_suffix : output->map_suffixes)
        textures.push_back({ &slice, output.get(), 
          path_to_utf8(replace_suffix(filename,
            output->default_map_suffix, map_suffix)), i++ });
    }
  return textures;
}

void output_textures(const Settings& settings,
    std::vector<Texture>& textures) {
  scheduler.for_each_parallel(textures,
    [&](Texture& texture) {
      if (!output_texture(settings, texture))
        texture.filename = { };
    });
}

} // namespace
