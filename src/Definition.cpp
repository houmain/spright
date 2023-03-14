
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
  static const auto s_map = []() {
    auto map = std::map<std::string, Definition, std::less<>>{ };
    const auto begin = to_int(Definition::none);
    const auto end = to_int(Definition::MAX);
    for (auto d = begin; d != end; ++d) {
      const auto definition = static_cast<Definition>(d);
      map[std::string(get_definition_name(definition))] = definition;
    }
    return map;
  }();
  const auto it = s_map.find(command);
  if (it == s_map.end())
    return Definition::none;
  return it->second;
}

std::string_view get_definition_name(Definition definition) {
  switch (definition) {
    case Definition::none:
    case Definition::MAX:
      break;

    case Definition::set: return "set";
    case Definition::group: return "group";
    case Definition::sheet: return "sheet";
    case Definition::output: return "output";
    case Definition::width: return "width";
    case Definition::height: return "height";
    case Definition::max_width: return "max-width";
    case Definition::max_height: return "max-height";
    case Definition::power_of_two: return "power-of-two";
    case Definition::square: return "square";
    case Definition::divisible_width: return "align-width";
    case Definition::allow_rotate: return "allow-rotate";
    case Definition::padding: return "padding";
    case Definition::duplicates: return "duplicates";
    case Definition::alpha: return "alpha";
    case Definition::pack: return "pack";
    case Definition::scale: return "scale";
    case Definition::debug: return "debug";
    case Definition::path: return "path";
    case Definition::input: return "input";
    case Definition::colorkey: return "colorkey";
    case Definition::grid: return "grid";
    case Definition::grid_cells: return "grid-cells";
    case Definition::grid_offset: return "grid-offset";
    case Definition::grid_spacing: return "grid-spacing";
    case Definition::row: return "row";
    case Definition::skip: return "skip";
    case Definition::span: return "span";
    case Definition::atlas: return "atlas";
    case Definition::maps: return "maps";
    case Definition::sprite: return "sprite";
    case Definition::id: return "id";
    case Definition::rect: return "rect";
    case Definition::pivot: return "pivot";
    case Definition::tag: return "tag";
    case Definition::data: return "data";
    case Definition::trim: return "trim";
    case Definition::trim_threshold: return "trim-threshold";
    case Definition::trim_margin: return "trim-margin";
    case Definition::trim_channel: return "trim-channel";
    case Definition::crop: return "crop";
    case Definition::crop_pivot: return "crop-pivot";
    case Definition::extrude: return "extrude";
    case Definition::min_bounds: return "min-bounds";
    case Definition::divisible_bounds: return "divisible-bounds";
    case Definition::common_bounds: return "common-bounds";
    case Definition::align: return "align";
    case Definition::align_pivot: return "align-pivot";
  }
  return "-";
}

Definition get_affected_definition(Definition definition) {
  switch (definition) {
    case Definition::none:
    case Definition::MAX:
    case Definition::group:
    case Definition::set:
    case Definition::input:
    case Definition::sprite:
    // allow sheets without sprites
    case Definition::sheet:
    // affect input and output
    case Definition::maps:
    // directly change state
    case Definition::row:
    case Definition::skip:
      return Definition::none;

    case Definition::output:
    case Definition::width:
    case Definition::height:
    case Definition::max_width:
    case Definition::max_height:
    case Definition::power_of_two:
    case Definition::square:
    case Definition::divisible_width:
    case Definition::allow_rotate:
    case Definition::padding:
    case Definition::duplicates:
    case Definition::pack:
      return Definition::sheet;

    case Definition::alpha:
    case Definition::scale:
    case Definition::debug:
      return Definition::output;

    case Definition::path:
    case Definition::colorkey:
    case Definition::grid:
    case Definition::grid_cells:
    case Definition::grid_offset:
    case Definition::grid_spacing:
    case Definition::atlas:
      return Definition::input;

    case Definition::id:
    case Definition::rect:
    case Definition::pivot:
    case Definition::tag:
    case Definition::data:
    case Definition::trim:
    case Definition::trim_threshold:
    case Definition::trim_margin:
    case Definition::trim_channel:
    case Definition::crop:
    case Definition::crop_pivot:
    case Definition::extrude:
    case Definition::min_bounds:
    case Definition::divisible_bounds:
    case Definition::common_bounds:
    case Definition::align:
    case Definition::align_pivot:
    case Definition::span:
      return Definition::sprite;
  }
  return Definition::none;
}

void apply_definition(Definition definition,
    std::vector<std::string_view>& arguments,
    State& state, Point& current_grid_cell,
    VariantMap& variables) {

  auto argument_index = 0u;
  const auto arguments_left = [&]() {
    return argument_index < arguments.size();
  };
  const auto check_string = [&]() {
    check(arguments_left(), "invalid argument count for ",
      get_definition_name(definition));
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
    if (auto b = to_bool(str))
      return b.value();
    error("invalid boolean value '", str, "'");
  };
  const auto check_real = [&]() {
    const auto str = check_string();
    if (auto value = to_real(str))
      return *value;
    error("invalid real value '", str, "'");
  };
  const auto check_variant = [&]() -> Variant {
    const auto str = check_string();
    if (auto b = to_bool(str))
      return b.value();
    if (auto r = to_real(str))
      return r.value();
    return std::string(str);
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
  const auto check_anchor = [&]() {
    // join expressions (e.g. middle - 7 top + 2)
    if (!std::all_of(arguments.begin(), arguments.end(),
        [](const auto& v) { return to_real(v).has_value(); }))
      join_expressions(&arguments);

    auto anchor = AnchorF{ { 0, 0 }, AnchorX::left, AnchorY::top };
    auto current_coord = &anchor.x;
    auto prev_coord = std::add_pointer_t<real>{ };
    auto expr = std::vector<std::string_view>();
    for (auto i = 0; i < 2; ++i) {
      split_expression(check_string(), &expr);
      if (const auto index = index_of(expr.front(),
          { "left", "center", "right" }); index >= 0) {
        anchor.anchor_x = static_cast<AnchorX>(index);
        current_coord = &anchor.x;
      }
      else if (const auto index = index_of(expr.front(),
          { "top", "middle", "bottom" }); index >= 0) {
        anchor.anchor_y = static_cast<AnchorY>(index);
        current_coord = &anchor.y;
      }
      else if (auto value = to_real(expr.front())) {
        *current_coord = *value;
      }
      else if (!expr.front().empty()) {
        error("invalid anchor value '", expr.front(), "'");
      }

      if (expr.size() % 2 != 1)
        error("invalid anchor expression");
      for (auto j = 1u; j < expr.size(); j += 2) {
        const auto value = to_real(expr[j + 1]);
        if (value && expr[j] == "+")
          *current_coord += *value;
        else if (value && expr[j] == "-")
          *current_coord -= *value;
        else
          error("invalid anchor expression");
      }

      if (prev_coord == current_coord)
        error("duplicate coordinate specified");
      prev_coord = current_coord;
      current_coord = (current_coord == &anchor.x ?
        &anchor.y : &anchor.x);
    }
    return anchor;
  };

  switch (definition) {
    case Definition::set: {
      auto& var = variables[std::string(check_string())];
      var = check_variant();
      break;
    }
    case Definition::group:
      // just for opening scopes, useful for additive definitions (e.g. tags)
      break;

     case Definition::sheet:
      state.sheet_id = check_string();
      break;

    case Definition::output:
      state.output_filenames.push_back(check_string());
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

    case Definition::divisible_width:
      state.divisible_width = check_uint();
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
      if (const auto index = index_of(string, 
          { "keep", "share", "drop" }); index >= 0)
        state.duplicates = static_cast<Duplicates>(index);
      else
        error("invalid duplicates value '", string, "'");
      break;
    }

    case Definition::alpha: {
      const auto string = check_string();
      if (const auto index = index_of(string, 
          { "keep", "clear", "bleed", "premultiply", "colorkey" }); index >= 0)
        state.alpha = static_cast<Alpha>(index);
      else
        error("invalid alpha value '", string, "'");

      if (state.alpha == Alpha::colorkey)
        state.alpha_colorkey = check_color();
      break;
    }

    case Definition::pack: {
      const auto string = check_string();
      if (const auto index = index_of(string, 
          { "binpack", "rows", "columns", "compact", "single", "layers", "keep" }); index >= 0)
        state.pack = static_cast<Pack>(index);
      else
        error("invalid pack method '", string, "'");
      break;
    }

    case Definition::scale:
      state.scale = check_real();
      check(state.scale >= 0.01 && state.scale < 100, "invalid scale");
      state.scale_filter = ResizeFilter::undefined;
      if (arguments_left()) {
        const auto string = check_string();
        if (const auto index = index_of(string, 
            { "default", "box", "triangle", "cubicspline",
                "catmullrom", "mitchell" }); index >= 0)
          state.scale_filter = static_cast<ResizeFilter>(index);
        else
          error("invalid scale filter '", string, "'");
      }
      break;

    case Definition::debug:
      state.debug = check_bool(true);
      break;

    case Definition::path:
      state.path = check_string();
      break;

    case Definition::input:
      state.source_filenames = path_to_utf8(check_path());
      current_grid_cell = { };
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
    case Definition::data: {
      auto& data = state.data[std::string(check_string())];
      data = check_variant();
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
      current_grid_cell.x = 0;
      current_grid_cell.y = check_uint();
      break;

    case Definition::skip:
      check(has_grid(state), "skip is only valid in grid");
      current_grid_cell.x += (arguments_left() ? check_uint() : 1);
      break;

    case Definition::span:
      check(has_grid(state), "span is only valid in grid");
      state.span = check_size(false);
      check(state.span.x > 0 && state.span.y > 0, "invalid span");
      break;

    case Definition::atlas:
      state.atlas_merge_distance = (arguments_left() ? check_uint() : 0);
      break;

    case Definition::maps:
      state.default_map_suffix = check_string();
      state.map_suffixes.clear();
      while (arguments_left())
        state.map_suffixes.emplace_back(check_string());
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

    case Definition::pivot:
      state.pivot = check_anchor();
      break;

    case Definition::trim: {
      const auto string = check_string();
      if (const auto index = index_of(string, 
          { "none", "rect", "convex" }); index >= 0)
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

    case Definition::crop_pivot:
      state.crop_pivot = check_bool(true);
      break;

    case Definition::extrude:
      state.extrude = {};
      state.extrude.count = (arguments_left() ? check_uint() : 1);
      if (arguments_left()) {
        const auto string = check_string();
        if (const auto index = index_of(string, 
            { "clamp", "mirror", "repeat" }); index >= 0)
          state.extrude.mode = static_cast<WrapMode>(index);
        else
          error("invalid extrude mode '", string, "'");
      }
      break;

    case Definition::min_bounds:
      state.min_bounds = check_size(true);
      break;

    case Definition::divisible_bounds:
      state.divisible_bounds = check_size(true);
      check(state.divisible_bounds.x >= 1 && state.divisible_bounds.y >= 1, 
        "invalid divisor");
      break;

    case Definition::common_bounds:
      state.common_bounds = (arguments_left() ? check_string() : "ALL");
      break;

    case Definition::align: {
      const auto anchor = check_anchor();
      state.align = {
        Point(anchor),
        anchor.anchor_x,
        anchor.anchor_y
      };
      break;
    }

    case Definition::align_pivot:
      state.align_pivot = (arguments_left() ? check_string() : "ALL");
      break;

    case Definition::MAX:
    case Definition::none:
      break;
  }

  check(!arguments_left(), "invalid argument count");
}

} // namespace
