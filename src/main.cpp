
#include "trimming.h"
#include "packing.h"
#include "output.h"
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

  auto [inputs, sprites, variables] = parse_definition(settings);  
  time_points.emplace_back(Clock::now(), "input");

  if (!settings.autocomplete) {
    trim_sprites(sprites);
    time_points.emplace_back(Clock::now(), "trimming");

    auto slices = pack_sprites(sprites);
    auto textures = get_textures(settings, slices);
    evaluate_expressions(settings, sprites, textures, variables);
    time_points.emplace_back(Clock::now(), "packing");    

    if (!settings.describe) {
      if (!settings.rebuild)
        update_last_source_written_times(slices);
      output_textures(textures);
      time_points.emplace_back(Clock::now(), "output textures");
    }
    output_description(settings, inputs, sprites, 
      slices, textures, variables);
    time_points.emplace_back(Clock::now(), "output description");
  }

  if (settings.verbose) {
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
