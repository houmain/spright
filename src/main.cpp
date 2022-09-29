
#include "trimming.h"
#include "packing.h"
#include "output.h"
#include "globbing.h"
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

  for_each_parallel(begin(textures), end(textures),
    [&](const Texture& texture) {
      const auto filename = utf8_to_path(
        texture.output->filename.get_nth_filename(texture.index));
      if (auto image = get_output_texture(settings, texture))
        save_image(image, filename);
      
      auto i = 0;
      for (const auto& layer_suffix : texture.output->layer_suffixes)
        if (auto image = get_output_texture(settings, texture, i++))
          save_image(image, replace_suffix(filename, 
            texture.output->default_layer_suffix, layer_suffix));
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
  std::cerr << ex.what() << std::endl;
  return 1;
}
