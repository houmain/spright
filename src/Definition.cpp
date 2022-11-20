
#include "Definition.h"
#include <charconv>
#include <sstream>
#include <utility>

namespace spright {

namespace {
  int index_of(std::string_view string, std::initializer_list<const char*> strings) {
    auto i = 0;
    for (auto s : strings) {
      if (string == s)
        return i;
      ++i;
    }
    return -1;
  }
} // namespace

bool has_grid(const State& state) {
  return !empty(state.grid) || !empty(state.grid_cells);
}

Definition get_definition(std::string_view command) {
  static const auto s_map = std::map<std::string, Definition, std::less<>>{
    { "output", Definition::output },
    { "width", Definition::width },
    { "height", Definition::height },
    { "max-width", Definition::max_width },
    { "max-height", Definition::max_height },
    { "power-of-two", Definition::power_of_two },
    { "square", Definition::square },
    { "align-width", Definition::align_width },
    { "allow-rotate", Definition::allow_rotate },
    { "padding", Definition::padding },
    { "duplicates", Definition::duplicates },
    { "alpha", Definition::alpha },
    { "pack", Definition::pack },
    { "group", Definition::group },
    { "path", Definition::path },
    { "input", Definition::input },
    { "colorkey", Definition::colorkey },
    { "tag", Definition::tag },
    { "grid", Definition::grid },
    { "grid-cells", Definition::grid_cells },
    { "grid-offset", Definition::grid_offset },
    { "grid-spacing", Definition::grid_spacing },
    { "row", Definition::row },
    { "sprite", Definition::sprite },
    { "id", Definition::id },
    { "skip", Definition::skip },
    { "span", Definition::span },
    { "atlas", Definition::atlas },
    { "layers", Definition::layers },
    { "rect", Definition::rect },
    { "pivot", Definition::pivot },
    { "trim", Definition::trim },
    { "trim-threshold", Definition::trim_threshold },
    { "trim-margin", Definition::trim_margin },
    { "trim-channel", Definition::trim_channel },
    { "crop", Definition::crop },
    { "extrude", Definition::extrude },
    { "common-divisor", Definition::common_divisor },
  };
  const auto it = s_map.find(command);
  if (it == s_map.end())
    return Definition::none;
  return it->second;
}

void apply_definition(State& state,
    Definition definition,
    std::vector<std::string_view>& arguments) {

  auto argument_index = 0u;
  const auto arguments_left = [&]() {
    return argument_index < arguments.size();
  };
  const auto check_string = [&]() {
    check(arguments_left(), "invalid argument count");
    return arguments[argument_index++];
  };
  const auto check_path = [&]() {
    return utf8_to_path(check_string());
  };
  const auto check_uint = [&]() {
    auto result = 0;
    const auto str = check_string();
    const auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
    check(ec == std::errc() && result >= 0, "invalid number");
    return result;
  };
  const auto check_bool = [&](bool default_to_true) {
    if (default_to_true && !arguments_left())
      return true;
    const auto str = check_string();
    if (str == "true") return true;
    if (str == "false") return false;
    error("invalid boolean value '", str, "'");
  };
  const auto check_size = [&](bool default_to_square) {
    const auto x = check_uint();
    return Size{ x, (arguments_left() || !default_to_square ? check_uint() : x) };
  };
  const auto check_rect = [&]() {
    return Rect{ check_uint(), check_uint(), check_uint(), check_uint() };
  };
  const auto check_color = [&]() {
    std::stringstream ss;
    ss << std::hex << check_string();
    auto color = RGBA{ };
    ss >> color.rgba;
    if (!color.a)
      color.a = 255;
    return color;
  };

  switch (definition) {
    case Definition::group:
      // just for opening scopes, useful for additive definitions (e.g. tags)
      break;

    case Definition::output:
      state.output = check_string();
      break;

    case Definition::width:
      state.width = check_uint();
      break;

    case Definition::height:
      state.height = check_uint();
      break;

    case Definition::max_width:
      state.max_width = check_uint();
      break;

    case Definition::max_height:
      state.max_height = check_uint();
      break;

    case Definition::power_of_two:
      state.power_of_two = check_bool(true);
      break;

    case Definition::square:
      state.square = check_bool(true);
      break;

    case Definition::align_width:
      state.align_width = check_uint();
      break;

    case Definition::allow_rotate:
      state.allow_rotate = check_bool(true);
      break;

    case Definition::padding:
      state.shape_padding =
        (arguments_left() ? check_uint() : 1);
      state.border_padding =
        (arguments_left() ? check_uint() : state.shape_padding);
      break;

    case Definition::duplicates: {
      const auto string = check_string();
      if (const auto index = index_of(string, { "keep", "share", "drop" }); index >= 0)
        state.duplicates = static_cast<Duplicates>(index);
      else
        error("invalid duplicates value '", string, "'");
      break;
    }

    case Definition::alpha: {
      const auto string = check_string();
      if (const auto index = index_of(string, { "keep", "clear", "bleed", "premultiply", "colorkey" }); index >= 0)
        state.alpha = static_cast<Alpha>(index);
      else
        error("invalid alpha value '", string, "'");

      if (state.alpha == Alpha::colorkey)
        state.alpha_colorkey = check_color();
      break;
    }

    case Definition::pack: {
      const auto string = check_string();
      if (const auto index = index_of(string, { "binpack", "compact", "single", "keep" }); index >= 0)
        state.pack = static_cast<Pack>(index);
      else
        error("invalid pack method '", string, "'");
      break;
    }

    case Definition::path:
      state.path = check_string();
      break;

    case Definition::input:
      state.sheet = path_to_utf8(check_path());
      state.current_grid_cell_x = { };
      state.current_grid_cell_y = { };
      break;

    case Definition::colorkey: {
      const auto guess = RGBA{ { 255, 255, 255, 0 } };
      state.colorkey = (arguments_left() ? check_color() : guess);
      break;
    }
    case Definition::tag: {
      auto& tag = state.tags[std::string(check_string())];
      tag = (arguments_left() ? check_string() : "");
      break;
    }
    case Definition::grid:
      state.grid = check_size(true);
      check(state.grid.x > 0 && state.grid.y > 0, "invalid grid");
      break;


    case Definition::grid_cells:
      // allow cell count in one dimension to be zero
      state.grid_cells = check_size(false);
      check((state.grid_cells.x >= 0 && state.grid_cells.y >= 0) &&
            (state.grid_cells.x > 0 || state.grid_cells.y > 0), "invalid grid");
      break;

    case Definition::grid_offset:
      state.grid_offset = check_size(true);
      break;

    case Definition::grid_spacing:
      state.grid_spacing = check_size(true);
      break;

    case Definition::row:
      check(has_grid(state), "offset is only valid in grid");
      state.current_grid_cell_x = 0;
      state.current_grid_cell_y = check_uint();
      break;

    case Definition::skip:
      check(has_grid(state), "skip is only valid in grid");
      state.current_grid_cell_x += (arguments_left() ? check_uint() : 1);
      break;

    case Definition::span:
      check(has_grid(state), "span is only valid in grid");
      state.span = check_size(false);
      check(state.span.x > 0 && state.span.y > 0, "invalid span");
      break;

    case Definition::atlas:
      state.atlas_merge_distance = (arguments_left() ? check_uint() : 0);
      break;

    case Definition::layers:
      state.default_layer_suffix = check_string();
      state.layer_suffixes.clear();
      while (arguments_left())
        state.layer_suffixes.emplace_back(check_string());
      break;

    case Definition::sprite:
      if (arguments_left())
        state.sprite_id = check_string();
      break;

    case Definition::id:
      state.sprite_id = check_string();
      break;

    case Definition::rect:
      state.rect = check_rect();
      break;

    case Definition::pivot: {
      // join expressions (e.g. middle - 7 top + 2)
      if (!std::all_of(arguments.begin(), arguments.end(), to_float))
        join_expressions(&arguments);

      state.pivot = { PivotX::left, PivotY::top };
      auto current_coord = &state.pivot_point.x;
      auto prev_coord = std::add_pointer_t<float>{ };
      auto expr = std::vector<std::string_view>();
      for (auto i = 0; i < 2; ++i) {
        split_expression(check_string(), &expr);
        if (const auto index = index_of(expr.front(), 
            { "left", "center", "right" }); index >= 0) {
          state.pivot.x = static_cast<PivotX>(index);
          current_coord = &state.pivot_point.x;
        }
        else if (const auto index = index_of(expr.front(), 
            { "top", "middle", "bottom" }); index >= 0) {
          state.pivot.y = static_cast<PivotY>(index);
          current_coord = &state.pivot_point.y;
        }
        else if (auto value = to_float(expr.front())) {
          *current_coord = *value;
        }
        else if (!expr.front().empty()) {
          error("invalid pivot value '", expr.front(), "'");
        }

        if (expr.size() % 2 != 1)
          error("invalid pivot expression");
        for (auto j = 1u; j < expr.size(); j += 2) {
          const auto value = to_float(expr[j + 1]);
          if (value && expr[j] == "+")
            *current_coord += *value;
          else if (value && expr[j] == "-")
            *current_coord -= *value;
          else
            error("invalid pivot expression");
        }

        if (prev_coord == current_coord)
          error("duplicate pivot coordinate specified");
        prev_coord = current_coord;
        current_coord = (current_coord == &state.pivot_point.x ? 
          &state.pivot_point.y : &state.pivot_point.x);
      }
      break;
    }

    case Definition::trim: {
      const auto string = check_string();
      if (const auto index = index_of(string, { "none", "rect", "convex" }); index >= 0)
        state.trim = static_cast<Trim>(index);
      else
        error("invalid trim value '", string, "'");
      break;
    }

    case Definition::trim_margin:
      state.trim_margin = (arguments_left() ? check_uint() : 1);
      break;

    case Definition::trim_threshold:
      state.trim_threshold = check_uint();
      check(state.trim_threshold >= 1 && state.trim_threshold <= 255, "invalid threshold");
      break;

    case Definition::trim_channel: {
      const auto channel = check_string();
      check(channel == "alpha" || channel == "gray", "invalid trim channel");
      state.trim_gray_levels = (channel == "gray");
      break;
    }

    case Definition::crop:
      state.crop = check_bool(true);
      break;

    case Definition::extrude:
      state.extrude = (arguments_left() ? check_uint() : 1);
      break;

    case Definition::common_divisor:
      state.common_divisor = check_size(true);
      check(state.common_divisor.x >= 1 && state.common_divisor.y >= 1, "invalid divisor");
      break;

    case Definition::none: break;
  }

  check(!arguments_left(), "invalid argument count");
}

} // namespace
