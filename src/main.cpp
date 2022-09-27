
#include "trimming.h"
#include "packing.h"
#include "output.h"
#include "globbing.h"
#include <iostream>
#include <chrono>

int main(int argc, const char* argv[]) try {
  using namespace spright;

  std::setlocale(LC_ALL, "en_US.UTF-8");

  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv)) {
    print_help_message(argv[0]);
    return 1;
  }

  using Clock = std::chrono::high_resolution_clock;
  auto time_points = std::vector<Clock::time_point>();
  const auto add_time_point = [&]() { time_points.push_back(Clock::now()); };
  add_time_point();

  auto sprites = parse_definition(settings);  
  add_time_point();

  for (auto& sprite : sprites)
    trim_sprite(sprite);
  add_time_point();

  const auto textures = pack_sprites(sprites);
  add_time_point();

  write_output_description(settings, sprites, textures);
  add_time_point();

  for_each_parallel(begin(textures), end(textures),
    [&](const Texture& texture) {
      const auto filename = 
        texture.output->filename.get_nth_filename(texture.index);
      if (auto image = get_output_texture(settings, texture))
        save_image(image, filename);
      
      auto i = 0;
      for (const auto& layer_suffix : texture.output->layer_suffixes)
        if (auto image = get_output_texture(settings, texture, i++))
          save_image(image, replace_suffix(filename, 
            texture.output->default_layer_suffix, layer_suffix));
    });
  add_time_point();

  if (settings.debug) {
    auto time_it = time_points.begin();
    const auto time_elapsed = [&]() {
      const auto start = *time_it++;
      const auto stop = *time_it;
      return std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    };
    std::cout <<
      "input: " << time_elapsed() << "ms, " <<
      "trimming: " << time_elapsed() << "ms, " <<
      "packing: " << time_elapsed() << "ms, " <<
      "output description: " << time_elapsed() << "ms, " <<
      "output textures: " << time_elapsed() << "ms" <<
      std::endl;
  }
  return 0;
}
catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
  return 1;
}
