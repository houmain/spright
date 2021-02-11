
#include "packing.h"
#include "output.h"
#include <iostream>

extern void test();

int main(int argc, const char* argv[]) try {

#if !defined(NDEBUG)
  test();
#endif

  auto settings = Settings{ };
  if (!interpret_commandline(settings, argc, argv) ||
      settings.input_file.empty()) {
    print_help_message(argv[0]);
    return 1;
  }

  auto sprites = parse_definition(settings);
  const auto textures = pack_sprites(sprites);
  for (const auto& texture : textures)
    output_texture(settings, texture);
  output_definition(settings, sprites);
}
catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
}
