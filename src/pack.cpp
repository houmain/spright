
#include "pack.h"
#include "rbp/MaxRectsBinPack.h"
#include "common.h"
#include <optional>
#include <cmath>

namespace {
  struct Run {
    std::vector<PackResult> sheets;
    int width;
    int height;
    int total_area;
  };

  bool is_better_than(const Run& a, const Run& b) {
    if (a.sheets.size() < b.sheets.size())
      return true;
    if (b.sheets.size() < a.sheets.size())
      return false;
    return (a.total_area < b.total_area);
  }

  int estimate_area(const std::vector<PackSize>& sizes) {
    auto area = 0;
    for (auto& size : sizes)
      area += (size.width * size.height);
    return area;
  }

  std::pair<int, int> correct_size(const PackSettings& settings, int width, int height) {
    width = std::max(width, settings.min_width);
    height = std::max(height, settings.min_height);
    for (auto i = 0; i < 2; ++i) {
      if (settings.power_of_two) {
        width = ceil_to_pot(width);
        height = ceil_to_pot(height);
      }
      if (settings.square) {
        width = height = std::max(width, height);
      }
      if (settings.align_width) {
        width = ceil(width, settings.align_width);
      }
    }
    return {
      std::min(width, settings.max_width),
      std::min(height, settings.max_height)
    };
  }

  std::pair<int, int> get_initial_size(const PackSettings& settings,
                                       const std::vector<PackSize>& sizes) {
    const auto area = estimate_area(sizes);
    auto width = 0;
    auto height = 0;
    if (settings.square || settings.max_width == settings.max_height) {
      width = height = static_cast<int>(std::sqrt(area));
    }
    else {
      if (settings.max_width < settings.max_height) {
        width = settings.max_width;
        height = area / width;
      }
      else {
        height = settings.max_height;
        width = area / height;
      }
    }
    return correct_size(settings, width, height);
  }

  bool optimize_run_size(const PackSettings& settings, const Run& best_run,
      int& run_width, int& run_height) {

    const auto last_succeeded = (best_run.width == run_width && best_run.height == run_height);
    if (!last_succeeded)
      return false;

    // TODO:
    if (best_run.sheets.size() > 1) {
      const auto& last_sheet = best_run.sheets.back();
      auto area = last_sheet.width * last_sheet.height;
      run_height += std::max(area / run_width * 3 / 2, 1);
    }
    else {
      run_height -= 1;
    }

    std::tie(run_width, run_height) = correct_size(settings, run_width, run_height);
    return true;
  }
}

std::vector<PackResult> pack(const PackSettings& settings, const std::vector<PackSize>& sizes) {

  using Method = rbp::MaxRectsBinPack::FreeRectChoiceHeuristic;
  const auto all_methods = {
    Method::RectBestShortSideFit,
    Method::RectBestLongSideFit,
    Method::RectBestAreaFit,
    Method::RectBottomLeftRule,
    Method::RectContactPointRule
  };
  const auto sole_method = { static_cast<Method>(settings.max_rects_method.value_or(0)) };
  const auto methods = (settings.max_rects_method.has_value() ? sole_method : all_methods);

  auto rbp_sizes = std::vector<rbp::RectSize>();
  rbp_sizes.reserve(sizes.size());
  for (auto i = 0; i < static_cast<int>(sizes.size()); ++i) {
    const auto size = sizes[static_cast<size_t>(i)];
    rbp_sizes.push_back(rbp::RectSize{ size.width, size.height, i });
  }

  auto best_run = std::optional<Run>{ };
  auto max_rects = rbp::MaxRectsBinPack();
  auto rbp_rects = std::vector<rbp::Rect>();
  rbp_rects.reserve(sizes.size());

  auto [run_width, run_height] = get_initial_size(settings, sizes);
  for (;;) {
    for (auto method : methods) {
      auto run = Run{ };
      auto run_rbp_sizes = rbp_sizes;
      auto cancelled = false;
      run.width = run_width;
      run.height = run_height;

      while (!run_rbp_sizes.empty()) {
        rbp_rects.clear();
        max_rects.Init(
          run_width - settings.border_padding * 2 + settings.over_allocate,
          run_height - settings.border_padding * 2 + settings.over_allocate,
          settings.allow_rotate);
        max_rects.Insert(run_rbp_sizes, rbp_rects, method);

        const auto [bottom, right] = max_rects.BottomRight();
        const auto [width, height] = correct_size(settings,
          bottom + settings.border_padding * 2 - settings.over_allocate,
          right + settings.border_padding * 2 - settings.over_allocate);

        auto& sheet = run.sheets.emplace_back(PackResult{ width , height, { } });
        run.total_area += (width * height);

        if (best_run && !is_better_than(run, *best_run)) {
          cancelled = true;
          break;
        }

        for (auto& rbp_rect : rbp_rects) {
          const auto& size = sizes[static_cast<size_t>(rbp_rect.id)];
          sheet.rects.push_back({
            size.id,
            rbp_rect.x + settings.border_padding,
            rbp_rect.y + settings.border_padding,
            (rbp_rect.width != size.width)
          });
        }
      }

      if (!cancelled)
        if (!best_run || is_better_than(run, *best_run))
          best_run = std::move(run);
    }

    if (!optimize_run_size(settings, *best_run, run_width, run_height))
      break;
  }

  return std::move(best_run->sheets);
}

