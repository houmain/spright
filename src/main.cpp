
#include "packing.h"
#include "output.h"
#include <iostream>
#include <chrono>

int main(int argc, const char* argv[]) try {

#if defined(_WIN32)
  setlocale(LC_ALL, ".UTF-8");
#endif

  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv)) {
    print_help_message(argv[0]);
    return 1;
  }

  using Clock = std::chrono::high_resolution_clock;
  const auto t0 = Clock::now();

  auto sprites = parse_definition(settings);  
  const auto t1 = Clock::now();

  const auto textures = pack_sprites(sprites);
  const auto t2 = Clock::now();

  write_output_description(settings, sprites, textures);
  const auto t3 = Clock::now();

  for_each_parallel(begin(textures), end(textures),
    [&](const PackedTexture& texture) {
      save_image(get_output_texture(settings, texture), texture.filename);
    });
  const auto t4 = Clock::now();

  if (settings.debug) {
    const auto to_ms = [](const auto& duration) {
      return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    };
    std::cout <<
      "input: " << to_ms(t1 - t0) << "ms, " <<
      "pack: " << to_ms(t2 - t1) << "ms, " <<
      "output description: " << to_ms(t3 - t2) << "ms, " <<
      "output textures: " << to_ms(t4 - t3) << "ms" <<
      std::endl;
  }
  return 0;
}
catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
  return 1;
}
