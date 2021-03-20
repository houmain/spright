
#include "packing.h"
#include "output.h"
#include <iostream>

int main(int argc, const char* argv[]) try {

#if defined(_WIN32)
  setlocale(LC_ALL, ".UTF-8");
#endif

  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv)) {
    print_help_message(argv[0]);
    return 1;
  }

  auto sprites = parse_definition(settings);
  const auto textures = pack_sprites(sprites);
  output_description(settings, sprites, textures);
  for (const auto& texture : textures)
    output_texture(settings, texture);
  return 0;
}
catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
  return 1;
}
