
#include "trimming.h"
#include "packing.h"
#include "output.h"
#include "globbing.h"
#include "debug.h"
#include "Scheduler.h"
#include <iostream>
#include <chrono>

int main(int argc, const char* argv[]) try {
  using namespace spright;

  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv)) {
    print_help_message(argv[0]);
    return 1;
  }

  using Clock = std::chrono::high_resolution_clock;
  auto time_points = std::vector<std::pair<Clock::time_point, const char*>>();
  time_points.emplace_back(Clock::now(), "begin");

  auto sprites = parse_definition(settings);  
  time_points.emplace_back(Clock::now(), "input");

  for (auto& sprite : sprites)
    trim_sprite(sprite);
  time_points.emplace_back(Clock::now(), "trimming");

  const auto textures = pack_sprites(sprites);
  time_points.emplace_back(Clock::now(), "packing");

  write_output_description(settings, sprites, textures);
  time_points.emplace_back(Clock::now(), "output description");

  auto scheduler = Scheduler();

  struct OutputLayer {
    const Texture* texture;
    std::filesystem::path filename;
    int layer_index;
  };
  auto output_layers = std::vector<OutputLayer>();
  for (const auto& texture : textures) {
    const auto filename = settings.output_path / utf8_to_path(
        texture.output->filename.get_nth_filename(texture.index));
    output_layers.push_back({ &texture, filename, -1 });

    auto i = 0;
    for (const auto& layer_suffix : texture.output->layer_suffixes)
      output_layers.push_back({ &texture, replace_suffix(filename,
          texture.output->default_layer_suffix, layer_suffix), i++ });
  }
  scheduler.for_each_parallel(begin(output_layers), end(output_layers),
    [&](const OutputLayer& output_layers) noexcept {
      const auto [texture, filename, layer_index] = output_layers;
      if (auto image = get_output_texture(*texture, layer_index)) {
        if (settings.debug)
          draw_debug_info(image, *texture);
        save_image(image, filename);
      }
    });
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
