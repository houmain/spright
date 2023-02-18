
#include "trimming.h"
#include "packing.h"
#include "output.h"
#include "globbing.h"
#include "debug.h"
#include "Scheduler.h"
#include <iostream>
#include <chrono>
#include <unordered_set>

namespace {
  using namespace spright;

  struct OutputMap {
    const Texture* texture;
    std::filesystem::path filename;
    int map_index;
  };

  std::vector<OutputMap> get_output_maps(const Settings& settings,
      const std::vector<Texture>& textures) {
    auto output_maps = std::vector<OutputMap>();
    for (const auto& texture : textures) {
      const auto filename = settings.output_path / utf8_to_path(texture.filename);
      output_maps.push_back({ &texture, filename, -1 });

      auto i = 0;
      for (const auto& map_suffix : texture.output->map_suffixes)
        output_maps.push_back({ &texture, replace_suffix(filename,
            texture.output->default_map_suffix, map_suffix), i++ });
    }
    return output_maps;
  }

  void update_last_source_written_time(Texture& texture) {
    auto last_write_time = get_last_write_time(texture.output->input_file);

    auto sources = std::unordered_set<const Image*>();
    for (const auto& sprite : texture.sprites)
      sources.insert(sprite.source.get());
    for (const auto& source : sources)
      last_write_time = std::max(last_write_time,
        get_last_write_time(source->path() / source->filename()));

    texture.last_source_written_time = last_write_time;
  }

  void remove_not_updated(std::vector<OutputMap>& output_maps) {
    output_maps.erase(std::remove_if(output_maps.begin(), output_maps.end(), 
      [&](const OutputMap& output_map) {
        return (try_get_last_write_time(output_map.filename) > 
                output_map.texture->last_source_written_time);
      }), output_maps.end());
  }

  void output_textures(const Settings& settings,
      const std::vector<OutputMap>& output_maps) {
    auto scheduler = Scheduler();
    scheduler.for_each_parallel(begin(output_maps), end(output_maps),
      [&](const OutputMap& output_maps) {
        const auto& texture = *output_maps.texture;
        const auto& filename = output_maps.filename;
        auto image = get_output_texture(texture, output_maps.map_index);
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
          scheduler.for_each_parallel(begin(scalings), end(scalings),
            [&](const Scaling& scaling) {
              const auto scale = scaling.scale;
              auto resized = resize_image(image, scale, scaling.resize_filter);
              if (settings.debug)
                draw_debug_info(resized, texture, scale);
              save_image(resized, add_suffix(filename, scaling.filename_suffix));
            });
        }
      });
  }
} // namespace

int main(int argc, const char* argv[]) try {
  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv)) {
    print_help_message(argv[0]);
    return 1;
  }

  using Clock = std::chrono::high_resolution_clock;
  auto time_points = std::vector<std::pair<Clock::time_point, const char*>>();
  time_points.emplace_back(Clock::now(), "begin");

  auto [sprites, variables] = parse_definition(settings);  
  time_points.emplace_back(Clock::now(), "input");

  for (auto& sprite : sprites)
    trim_sprite(sprite);
  time_points.emplace_back(Clock::now(), "trimming");

  auto textures = pack_sprites(sprites);
  time_points.emplace_back(Clock::now(), "packing");

  evaluate_expressions(settings, sprites, textures, variables);
  write_output_description(settings, sprites, textures, variables);
  time_points.emplace_back(Clock::now(), "output description");

  auto output_maps = get_output_maps(settings, textures);
  if (!settings.rebuild) {
    for (auto& texture : textures)
      update_last_source_written_time(texture);
    remove_not_updated(output_maps);
  }
  output_textures(settings, output_maps);
  time_points.emplace_back(Clock::now(), "output textures");

  if (settings.debug) {
    for (auto i = 1u; i < time_points.size(); ++i)
      std::cout << (i > 1 ? ", " : "") << time_points[i].second << ": " << 
        std::chrono::duration_cast<std::chrono::milliseconds>(
          time_points[i].first - time_points[i - 1].first).count() << "ms";
    std::cout << std::endl;
  }
  return 0;
}
catch (const std::exception& ex) {
  std::cerr << "spright: " << ex.what() << std::endl;
  return 1;
}
