
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

  Image compose_image(const Texture& texture, int map_index) {
    auto target = Image(texture.width, texture.height, RGBA{ });

    auto copied_sprite = false;
    for (const auto& sprite : texture.sprites)
      copied_sprite |= copy_sprite(target, sprite, map_index);
    if (!copied_sprite)
      return { };

    process_alpha(target, texture);

    return target;
  }

  struct OutputTask {
    const Texture* texture;
    std::filesystem::path filename;
    int map_index;
  };

  std::vector<OutputTask> get_output_tasks(const Settings& settings,
      const std::vector<Texture>& textures) {
    auto output_tasks = std::vector<OutputTask>();
    for (const auto& texture : textures) {
      const auto filename = settings.output_path / utf8_to_path(texture.filename);
      output_tasks.push_back({ &texture, filename, -1 });

      auto i = 0;
      for (const auto& map_suffix : texture.output->map_suffixes)
        output_tasks.push_back({ &texture, replace_suffix(filename,
            texture.output->default_map_suffix, map_suffix), i++ });
    }
    return output_tasks;
  }

  void remove_not_updated(std::vector<OutputTask>& output_tasks) {
    output_tasks.erase(std::remove_if(output_tasks.begin(), output_tasks.end(), 
      [&](const OutputTask& output_task) {
        return (try_get_last_write_time(output_task.filename) > 
                output_task.texture->last_source_written_time);
      }), output_tasks.end());
  }

  void output_image(const Settings& settings, const OutputTask& output_task) {
    const auto& texture = *output_task.texture;
    const auto& filename = output_task.filename;
    auto image = compose_image(texture, output_task.map_index);
    if (!image)
      return;

    const auto& output = *texture.output;
    const auto& scalings = output.scalings;
    if (scalings.empty()) {
      if (settings.debug)
        draw_debug_info(image, texture);
      save_image(image, filename);
    }
    else {
      scheduler->for_each_parallel(begin(scalings), end(scalings),
        [&](const Scaling& scaling) {
          const auto scale = scaling.scale;
          auto resized = resize_image(image, scale, scaling.resize_filter);
          if (settings.debug)
            draw_debug_info(resized, texture, scale);
          save_image(resized, add_suffix(filename, scaling.filename_suffix));
        });
    }
  }
} // namespace

void output_textures(const Settings& settings,
    std::vector<Texture>& textures) {

  auto output_tasks = get_output_tasks(settings, textures);
  if (!settings.rebuild) {
    for (auto& texture : textures)
      update_last_source_written_time(texture);
    remove_not_updated(output_tasks);
  }

  scheduler->for_each_parallel(begin(output_tasks), end(output_tasks),
    [&](const OutputTask& output_task) {
      output_image(settings, output_task);
    });
}

} // namespace
