
#include "InputParser.h"
#include <csignal>

template<typename A, typename B>
void eq(const A& a, const B& b) {
  if (!(a == b))
    std::raise(SIGINT);
}

void test() {
  auto parser = InputParser(Settings{ });
  auto input = std::stringstream();
  input << R"(
    sheet "Items.png"
      grid 16 16
      trim none
      tag "A"
        tag "B"
        sprite has_A_B
        trim crop
          tag "C"
          sprite has_A_B_C
            trim trim
        sprite has_A_B
      sprite has_A_D_E
        tag "D"
          trim trim
            tag "F"
          tag "E"
      tag "G"
        sprite has_A_G
  )";
  parser.parse(input);
  const auto& sprites = parser.sprites();
  eq(sprites.size(), 5u);

  eq(sprites[0].id, "has_A_B");
  eq(sprites[0].tags.size(), 2u);
  eq(sprites[0].trim, Trim::none);

  eq(sprites[1].id, "has_A_B_C");
  eq(sprites[1].tags.size(), 3u);
  eq(sprites[1].trim, Trim::trim);

  eq(sprites[2].id, "has_A_B");
  eq(sprites[2].tags.size(), 2u);
  eq(sprites[2].tags.count("B"), 1u);
  eq(sprites[2].trim, Trim::crop);

  eq(sprites[3].id, "has_A_D_E");
  eq(sprites[3].tags.size(), 3u);
  eq(sprites[3].tags.count("B"), 0u);
  eq(sprites[3].tags.count("E"), 1u);
  eq(sprites[3].trim, Trim::trim);

  eq(sprites[4].id, "has_A_G");
  eq(sprites[4].tags.size(), 2u);
  eq(sprites[4].tags.count("B"), 0u);
  eq(sprites[4].tags.count("G"), 1u);
  eq(sprites[4].trim, Trim::none);
}
