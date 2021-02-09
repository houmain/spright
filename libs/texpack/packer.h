#pragma once

#include <vector>

namespace pkr
{
	struct Params
  {
    bool pot;
    bool rotate;
		int padding;
		int width;
		int height;
		bool max_size;
	};

  struct Sprite
  {
    int id;
    int x;
    int y;
    int width;
    int height;
    bool rotated;
  };

  struct Result
  {
    int width;
    int height;
    std::vector<Sprite> sprites;
  };

  std::vector<Result> pack(const Params &params, std::vector<Sprite>& sprites);
}
