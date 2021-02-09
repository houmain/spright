
#include "packer.h"
#include "rbp/MaxRects.h"
#include <cstring>
#include <cmath>
#include <cstdint>

#define countof(x) (sizeof(x) / sizeof(x[0]))

namespace pkr
{

struct Packer
{
  const Params& params;

  std::vector<Sprite>& input_sprites;
  std::vector<rbp::RectSize> input_rects;

  Packer(const Params& params, std::vector<Sprite>& sprites)
    : params(params)
    , input_sprites(sprites)
  {
    input_rects.reserve(sprites.size());
    for (const auto& sprite : sprites)
      input_rects.push_back({ sprite.width, sprite.height });
  }

  bool has_fixed_size()
  {
    return params.width > 0 && params.height > 0 && !params.max_size;
  }

  bool can_enlarge(int width, int height)
  {
    return !has_fixed_size() && (!params.max_size ||
      width < params.width || height < params.height);
  }

  void calculate_initial_size(const std::vector<size_t> &rect_indices, int *w, int *h)
  {
    if (has_fixed_size())
    {
      *w = params.width;
      *h = params.height;
      return;
    }

    uint64_t area = 0;

    for (size_t i = 0; i < rect_indices.size(); i++)
    {
      const rbp::RectSize &rc = input_rects[rect_indices[i]];
      area += rc.width * rc.height;
    }

    int n = 1;
    int size = std::ceil(std::sqrt((double)area));

    while (n < size)
      n <<= 1;

    *w = *h = n;

    if (params.max_size)
    {
      *w = std::min(n, params.width);
      *h = std::min(n, params.height);
    }
  }

  std::vector<Result*> compute_results()
  {
    int modes[] = {
      rbp::MaxRects::BottomLeft,
      rbp::MaxRects::ShortSide,
      rbp::MaxRects::LongSide,
      rbp::MaxRects::BestArea,
      rbp::MaxRects::ContactPoint
    };

    std::vector<Result*> best;
    uint64_t best_area = 0;

    for (size_t i = 0; i < countof(modes); i++)
    {
      std::vector<Result*> res = compute_result(modes[i]);

      uint64_t area = 0;

      for (size_t j = 0; j < res.size(); j++)
        area += res[j]->width * res[j]->height;

      if (best.size() == 0 || res.size() < best.size() ||
        (res.size() == best.size() && area < best_area))
      {
        for (size_t j = 0; j < best.size(); j++)
          delete best[j];

        best.swap(res);
        best_area = area;
      }
      else
      {
        for (size_t j = 0; j < res.size(); j++)
          delete res[j];
      }
    }
    return best;
  }

  std::vector<Result*> compute_result(int mode)
  {
    std::vector<Result*> results;

    std::vector<rbp::Rect> result_rects;
    std::vector<size_t> result_indices;
    std::vector<size_t> input_indices(input_rects.size());
    std::vector<size_t> rects_indices;

    for (size_t i = 0; i < input_rects.size(); i++)
      input_indices[i] = i;

    int w = 0;
    int h = 0;

    calculate_initial_size(input_indices, &w, &h);

    while (input_indices.size() > 0)
    {
      rects_indices = input_indices;

      rbp::MaxRects packer(w - params.padding, h - params.padding, params.rotate);

      packer.insert(mode, input_rects, rects_indices, result_rects, result_indices);

      bool add_result = false;

      if (rects_indices.size() > 0)
      {
        if (can_enlarge(w, h))
        {
          if (params.max_size)
          {
            int *x = 0;

            if (w > h)
              x = h < params.height ? &h : &w;
            else
              x = w < params.width ? &w : &h;

            int max = x == &w ? params.width : params.height;

            *x = std::min(*x * 2, max);
          }
          else
          {
            if (w > h)
              h *= 2;
            else
              w *= 2;
          }
        }
        else
        {
          add_result = true;
          input_indices.swap(rects_indices);
        }
      }
      else
      {
        add_result = true;
        input_indices.clear();
      }

      if (add_result)
      {
        Result *result = new Result();

        result->width = w;
        result->height = h;

        result->sprites.resize(result_rects.size());

        int xmin = w;
        int xmax = 0;
        int ymin = h;
        int ymax = 0;

        for (size_t i = 0; i < result_rects.size(); i++)
        {
          size_t index = result_indices[i];

          const rbp::RectSize &base_rect = input_rects[index];
          const Sprite &base_sprite = input_sprites[index];

          Sprite &sprite = result->sprites[i];

          sprite = base_sprite;
          sprite.x = result_rects[i].x + params.padding;
          sprite.y = result_rects[i].y + params.padding;
          sprite.rotated = (result_rects[i].width != base_rect.width);

          xmin = std::min(xmin, result_rects[i].x);
          xmax = std::max(xmax, result_rects[i].x + result_rects[i].width);
          ymin = std::min(ymin, result_rects[i].y);
          ymax = std::max(ymax, result_rects[i].y + result_rects[i].height);
        }

        if (!has_fixed_size() && !params.pot)
        {
          result->width = (xmax - xmin) + params.padding;
          result->height = (ymax - ymin) + params.padding;

          if (xmin > 0 || ymin > 0)
          {
            for (size_t i = 0; i < result->sprites.size(); i++)
            {
              result->sprites[i].x -= xmin;
              result->sprites[i].y -= ymin;
            }
          }
        }

        results.push_back(result);

        if (input_indices.size() > 0)
          calculate_initial_size(input_indices, &w, &h);
      }
    }

    return results;
  }
};

std::vector<Result> pack(const Params &params, std::vector<Sprite>& sprites) {
  Packer packer(params, sprites);
  std::vector<Result> results;
  for (auto result : packer.compute_results()) {
    results.push_back(std::move(*result));
    delete result;
  }
  return results;
}

}
