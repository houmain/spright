
#include "input.h"
#include <charconv>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace {
  std::ostream* g_autocomplete_output;
  int g_line_number = 1;
  std::vector<Sprite> g_sprites;
  int g_sprites_in_current_sheet;
  Point g_current_offset;

  enum class Definition {
    none,
    sheet,
    colorkey,
    grid,
    offset,
    sprite,
    rect,
    skip,
    pivot,
    tag,
    margin,
    trim,
  };

  struct State {
    Definition definition{ };
    int level;
    std::string indent;

    std::string sheet;
    RGBA colorkey{ };
    std::map<std::string, std::string> tags;
    std::string sprite;
    Size grid{ };
    Pivot pivot{ PivotX::center, PivotY::middle };
    PointF pivot_point{ };
    Rect rect{ };
    int margin{ };
    Trim trim{ };
  };

  Definition get_definition(std::string_view command) {
    static const auto s_map = std::map<std::string, Definition, std::less<>>{
      { "sheet", Definition::sheet },
      { "colorkey", Definition::colorkey },
      { "tag", Definition::tag },
      { "grid", Definition::grid },
      { "offset", Definition::offset },
      { "sprite", Definition::sprite },
      { "skip", Definition::skip },
      { "rect", Definition::rect },
      { "pivot", Definition::pivot },
      { "margin", Definition::margin },
      { "trim", Definition::trim },
    };
    const auto it = s_map.find(command);
    if (it == s_map.end())
      return Definition::none;
    return it->second;
  }

  [[noreturn]] void error(std::string message) {
    throw std::runtime_error(message + " in line " + std::to_string(g_line_number));
  }

  void check(bool condition, std::string_view message) {
    if (!condition)
      error(std::string(message));
  }

  ImagePtr get_sheet(const std::string& filename) {
    static auto s_sheets = std::map<std::string, ImagePtr>();
    auto& sheet = s_sheets[filename];
    if (!sheet) {
      auto image = Image(filename);

      if (is_opaque(image)) {
        const auto color_key = guess_color_key(image);
        replace_color(image, color_key, RGBA{ });
      }

      sheet = std::make_shared<Image>(std::move(image));
    }
    return sheet;
  }

  void sprite_ends(State& state) {
    check(!state.sheet.empty(), "sprite not on sheet");

    // generate rect from grid
    if (empty(state.rect) && !empty(state.grid)) {
      state.rect = {
        g_current_offset.x, g_current_offset.y,
        state.grid.x, state.grid.y
      };
      g_current_offset.x += state.grid.x;
    }

    auto sprite = Sprite{ };
    sprite.id = (!state.sprite.empty() ? state.sprite :
      "sprite_" + std::to_string(g_sprites.size()));
    sprite.source = get_sheet(state.sheet);
    sprite.source_rect = state.rect;
    sprite.pivot = state.pivot;
    sprite.pivot_point = state.pivot_point;
    sprite.margin = state.margin;
    sprite.trim = state.trim;
    sprite.tags = state.tags;
    g_sprites.push_back(std::move(sprite));

    ++g_sprites_in_current_sheet;
  }

  void autocomplete_separate_sprites(State& state, std::ostream& os) {
    // TODO:
  }

  void autocomplete_grid_sprites(State& state, std::ostream& os) {
    const auto floor = [](int v, int q) { return (v / q) * q; };
    const auto ceil = [](int v, int q) { return ((v + q - 1) / q) * q; };

    const auto& sheet = *get_sheet(state.sheet);
    const auto bounds = get_used_bounds(sheet);
    const auto grid = state.grid;
    const auto x0 = floor(bounds.x, grid.x) / grid.x;
    const auto y0 = floor(bounds.y, grid.y) / grid.y;
    const auto x1 = ceil(bounds.x + bounds.w, grid.x) / grid.x;
    const auto y1 = ceil(bounds.y + bounds.h, grid.y) / grid.y;

    state.rect = { };
    for (auto y = y0; y < y1; ++y) {
      auto output_offset = false;
      auto skipped = 0;
      for (auto x = x0; x < x1; ++x) {
        g_current_offset.x = x * state.grid.x;
        g_current_offset.y = y * state.grid.y;
        state.rect = {
          g_current_offset.x, g_current_offset.y,
          state.grid.x, state.grid.y
        };

        if (is_fully_transparent(sheet, state.rect)) {
          ++skipped;
          continue;
        }
        if (!std::exchange(output_offset, true)) {
          if (x0 || y)
            os << state.indent << "offset " << x0 << " " << y << "\n";
        }

        if (skipped > 0) {
          os << state.indent << "skip";
          if (skipped > 1)
            os << " " << skipped;
          os << "\n";
          skipped = 0;
        }

        os << state.indent << "sprite\n";

        sprite_ends(state);
      }
    }
  }

  void autocomplete_unaligned_sprites(State& state, std::ostream& os) {
    const auto& sheet = *get_sheet(state.sheet);
    for (const auto& rect : find_islands(sheet)) {
      os << state.indent << "sprite \n";
      os << state.indent << "  rect "
        << rect.x << " " << rect.y << " "
        << rect.w << " " << rect.h << "\n";

      state.rect = rect;
      sprite_ends(state);
    }
  }

  void sheet_ends(State& state) {
    if (g_autocomplete_output && !g_sprites_in_current_sheet) {
      auto& os = *g_autocomplete_output;
      if (state.sheet.find('%') != std::string::npos) {
        autocomplete_separate_sprites(state, os);
      }
      else if (!empty(state.grid)) {
        autocomplete_grid_sprites(state, os);
      }
      else {
        autocomplete_unaligned_sprites(state, os);
      }
    }
    g_sprites_in_current_sheet = { };
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
    const auto is_number_following = [&]() {
      if (!arguments_left())
        return false;
      auto result = 0;
      const auto str = arguments[argument_index];
      return (std::from_chars(str.data(), str.data() + str.size(), result).ec == std::errc());
    };
    const auto check_int = [&]() {
      auto result = 0;
      auto str = check_string();
      const auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), result);
      check(ec == std::errc(), "invalid number");
      return result;
    };
    const auto check_float = [&]() {
      return std::stof(std::string(check_string()));
    };
    const auto check_size = [&]() {
      return Size{ check_int(), check_int() };
    };
    const auto check_rect = [&]() {
      return Rect{ check_int(), check_int(), check_int(), check_int() };
    };
    const auto check_enum = [&](auto e, std::initializer_list<const char*> strings) {
      const auto string = check_string();
      auto i = 0;
      for (auto s : strings) {
        if (string == s)
          return decltype(e){ i };
        ++i;
      }
      error("invalid enum value " + std::string(string));
    };

    switch (definition) {
      case Definition::sheet:
        state.sheet = check_string();
        g_current_offset = { };
        break;

      case Definition::tag: {
        auto& tag = state.tags[std::string(check_string())];
        tag = (arguments_left() ? check_string() : "");
        break;
      }
      case Definition::grid:
        state.grid = check_size();
        break;

      case Definition::offset:
        check(!empty(state.grid), "offset is only valid in grid");
        g_current_offset.x = static_cast<int>(check_float() * static_cast<float>(state.grid.x));
        g_current_offset.y = static_cast<int>(check_float() * static_cast<float>(state.grid.y));
        break;

      case Definition::skip:
        check(!empty(state.grid), "skip is only valid in grid");
        g_current_offset.x += (arguments_left() ? check_int() : 1) * state.grid.x;
        break;

      case Definition::sprite:
        if (arguments_left())
          state.sprite = check_string();
        break;

      case Definition::rect:
        state.rect = check_rect();
        break;

      case Definition::pivot:
        state.pivot = { PivotX::custom, PivotY::custom };
        if (is_number_following())
          state.pivot_point.x = check_float();
        else
          state.pivot.x = check_enum(PivotX{ }, { "left", "center", "right" });

        if (is_number_following())
          state.pivot_point.y = check_float();
        else
          state.pivot.y = check_enum(PivotY{ }, { "top", "middle", "bottom" });
        break;

      case Definition::margin:
        state.margin = check_int();
        break;

      case Definition::trim:
        state.trim = (!arguments_left() ? Trim::trim :
          check_enum(Trim{ }, { "none", "trim", "crop" }));
        break;

      case Definition::none: break;
    }

    check(!arguments_left(), "invalid argument count");
  }

  bool has_implicit_scope(Definition definition) {
    switch (definition) {
      case Definition::sheet:
      case Definition::sprite:
        return true;
      default:
        return false;
    }
  }

  void scope_ends(State& state) {
    switch (state.definition) {
      case Definition::sheet:
        sheet_ends(state);
        break;
      case Definition::sprite:
        sprite_ends(state);
        break;
      default:
        break;
    }
  }
} // namespace

std::vector<Sprite> parse_definition(const Settings& settings) {
  auto input = std::fstream(settings.input_file, std::ios::in);
  if (!input.good())
    error("opening file '" + settings.input_file.u8string() + "' failed");

  auto detected_indetation = std::string();
  auto scope_stack = std::vector<State>();
  scope_stack.emplace_back();
  scope_stack.back().level = -1;

  auto autocomplete_output = std::stringstream();
  auto autocomplete_space = std::stringstream();
  if (settings.autocomplete)
    g_autocomplete_output = &autocomplete_output;

  const auto pop_scope_stack = [&](int level) {
    for (auto last = scope_stack.rbegin(); ; ++last) {
      if (has_implicit_scope(last->definition) && level <= last->level) {
        auto& state = scope_stack.back();
        state.definition = last->definition;

        // add indentation before autocompleting in implicit scope
        if (&*last == &scope_stack.back())
          state.indent += detected_indetation;

        scope_ends(state);
      }
      else if (level >= last->level) {
        scope_stack.erase(last.base(), scope_stack.end());
        return;
      }
    }
  };

  auto buffer = std::string();
  auto arguments = std::vector<std::string_view>();
  for (; !input.eof(); ++g_line_number) {
    std::getline(input, buffer);

    auto line = ltrim(buffer);
    if (line.empty() || starts_with(line, "#")) {
      if (settings.autocomplete) {
        if (input.eof())
          autocomplete_output << autocomplete_space.str();
        else
          autocomplete_space << buffer << '\n';
      }
      continue;
    }

    split_arguments(line, &arguments);
    const auto definition = get_definition(arguments[0]);
    check(definition != Definition::none, "invalid definition");
    arguments.erase(arguments.begin());

    const auto level = static_cast<int>(buffer.size() - line.size());
    pop_scope_stack(level);

    if (level > scope_stack.back().level || has_implicit_scope(definition))
      scope_stack.push_back(scope_stack.back());

    auto& state = scope_stack.back();
    state.definition = definition;
    state.level = level;
    state.indent = std::string(begin(buffer), begin(buffer) + level);

    if (detected_indetation.empty() && !state.indent.empty())
      detected_indetation = state.indent;

    apply_definition(state, definition, arguments);

    if (settings.autocomplete) {
      const auto space = autocomplete_space.str();
      autocomplete_output << space << buffer << '\n';
      autocomplete_space = { };
    }
  }

  pop_scope_stack(-1);

  if (settings.autocomplete) {
    input.clear();
    const auto output = autocomplete_output.str();
    if (static_cast<int>(output.size()) != input.tellg()) {
      input.close();
      input = std::fstream(settings.input_file, std::ios::out);
      if (!input.good())
        error("opening file '" + settings.input_file.u8string() + "' failed");
      input << output;
    }
  }
  return std::move(g_sprites);
}
