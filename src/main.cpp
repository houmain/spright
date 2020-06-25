
#include "packing.h"
#include "output.h"
#include <iostream>

int main(int argc, const char* argv[]) try {
  auto settings = Settings{
  };
  if (!interpret_commandline(settings, argc, argv) ||
      settings.input_file.empty()) {
    print_help_message(argv[0]);
    return 1;
  }

  auto sprites = parse_definition(settings);
  pack_sprite_sheet(settings, sprites);
  output_definition(settings, sprites);
}
catch (const std::exception& ex) {
  std::cerr << ex.what() << std::endl;
}
