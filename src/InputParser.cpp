
#include "InputParser.h"
#include "globbing.h"
#include <charconv>
#include <algorithm>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  const auto default_output_name = "spright{0-}.png";
  const auto default_sprite_id = "sprite_{{ index }}";

  ImagePtr try_get_layer(const ImagePtr& sheet, 
      const std::string& default_layer_suffix, 
      const std::string& layer_suffix) {

    auto layer_filename = replace_suffix(sheet->filename(), 
      default_layer_suffix, layer_suffix);

    if (std::filesystem::exists(layer_filename))
      return std::make_shared<Image>(sheet->path(), layer_filename);
    return { };
  }

  bool has_layer_suffix(const std::string& filename,
      const std::vector<std::string>& layer_suffixes) {
    for (const auto& suffix : layer_suffixes)
      if (has_suffix(filename, suffix))
        return true;
    return false;
  }

  bool has_implicit_scope(Definition definition) {
    return (definition == Definition::output ||
            definition == Definition::input ||
            definition == Definition::sprite);
  }

  void validate_sprite(const Sprite& sprite) {
    if (sprite.source_rect != intersect(sprite.source->bounds(), sprite.source_rect)) {
      auto message = std::stringstream();
      message << "sprite";
      if (!sprite.id.empty())
        message << " '" << sprite.id << "' ";
      message << "(" <<
        sprite.source_rect.x << ", " <<
        sprite.source_rect.y << ", " <<
        sprite.source_rect.w << ", " <<
        sprite.source_rect.h << ") outside '" << path_to_utf8(sprite.source->filename()) << "' bounds";
      error(message.str());
    }
  }
} // namespace

OutputPtr InputParser::get_output(const State& state) {
  auto& output = m_outputs[std::filesystem::weakly_canonical(state.output)];
  if (!output) {
    update_applied_definitions(Definition::output);
    output = std::make_shared<Output>(Output{
      m_input_file,
      FilenameSequence(path_to_utf8(state.output)),
      state.default_layer_suffix,
      state.layer_suffixes,
      state.width,
      state.height,
      state.max_width,
      state.max_height,
      state.power_of_two,
      state.square,
      state.align_width,
      state.allow_rotate,
      state.border_padding,
      state.shape_padding,
      state.duplicates,
      state.alpha,
      state.alpha_colorkey,
      state.pack,
      state.scalings,
    });
  }
  return output;
}

ImagePtr InputParser::get_sheet(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey) {
  auto& sheet = m_sheets[std::filesystem::weakly_canonical(path / filename)];
  if (!sheet) {
    auto image = Image(path, filename);

    if (colorkey != RGBA{ }) {
      if (!colorkey.a)
        colorkey = guess_colorkey(image);
      replace_color(image, colorkey, RGBA{ });
    }

    sheet = std::make_shared<Image>(std::move(image));
  }
  return sheet;
}

ImagePtr InputParser::get_sheet(const State& state, int index) {
  return get_sheet(state.path,
    utf8_to_path(state.sheet.get_nth_filename(index)),
    state.colorkey);
}

ImagePtr InputParser::get_sheet(const State& state) {
  return get_sheet(state, m_current_sequence_index);
}

LayerVectorPtr InputParser::get_layers(const State& state, const ImagePtr& sheet) {
  if (state.layer_suffixes.empty())
    return { };

  auto it = m_layers.find(sheet);
  if (it == m_layers.end()) {
    auto layers = std::vector<ImagePtr>();
    for (const auto& layer_suffix : state.layer_suffixes)
      layers.push_back(try_get_layer(sheet, 
        state.default_layer_suffix, layer_suffix));
    it = m_layers.emplace(sheet, 
      std::make_shared<decltype(layers)>(std::move(layers))).first;
  }
  return it->second;
}

void InputParser::sprite_ends(State& state) {
  check(!state.sheet.empty(), "sprite not on input");
  update_applied_definitions(Definition::sprite);

  // generate rect from grid
  if (empty(state.rect) && has_grid(state)) {
    deduce_grid_size(state);
    state.rect = {
      state.grid_offset.x + (state.grid.x + state.grid_spacing.x) * m_current_grid_cell.x,
      state.grid_offset.y + (state.grid.y + state.grid_spacing.y) * m_current_grid_cell.y,
      state.grid.x * state.span.x,
      state.grid.y * state.span.y
    };
    m_current_grid_cell.x += state.span.x;
  }

  auto sprite = Sprite{ };
  sprite.index = to_int(m_sprites.size());
  sprite.input_sprite_index = m_sprites_in_current_sheet;
  sprite.id = state.sprite_id;
  sprite.output = get_output(state);
  sprite.source = get_sheet(state);
  sprite.layers = get_layers(state, sprite.source);
  sprite.source_rect = (!empty(state.rect) ?
    state.rect : sprite.source->bounds());
  sprite.pivot = state.pivot;
  sprite.pivot_point = state.pivot_point;
  sprite.trim = state.trim;
  sprite.trim_margin = state.trim_margin;
  sprite.trim_threshold = state.trim_threshold;
  sprite.trim_gray_levels = state.trim_gray_levels;
  sprite.crop = state.crop;
  sprite.crop_pivot = state.crop_pivot;
  sprite.extrude = state.extrude;
  sprite.common_divisor = state.common_divisor;
  sprite.tags = state.tags;
  sprite.data = state.data;
  validate_sprite(sprite);
  m_sprites.push_back(std::move(sprite));

  if (state.sheet.is_sequence())
    ++m_current_sequence_index;
  ++m_sprites_in_current_sheet;
}

void InputParser::deduce_globbing_sheets(State& state) {
  for (const auto& sequence : glob_sequences(state.path, state.sheet.filename())) {
    if (has_layer_suffix(sequence.filename(), state.layer_suffixes))
      continue;

    state.sheet = sequence;
    sheet_ends(state);
  }
}

void InputParser::deduce_sequence_sprites(State& state) {
  auto error = std::error_code{ };
  if (state.sheet.is_infinite_sequence()) {
    for (auto i = 0; ; ++i)
      if (!std::filesystem::exists(state.path / state.sheet.get_nth_filename(i), error)) {
        state.sheet.set_count(i);
        break;
      }
    if (state.sheet.is_infinite_sequence())
      return;
  }

  for (auto i = 0; i < state.sheet.count(); ++i) {
    const auto sheet = get_sheet(state, i);
    state.rect = sheet->bounds();
    sprite_ends(state);
  }
}

void InputParser::deduce_grid_size(State& state) {
  if (!empty(state.grid) ||
      empty(state.grid_cells))
    return;

  const auto sheet = get_sheet(state);
  const auto sx = (state.grid_cells.x > 0 ? 
    div_ceil(sheet->width() - state.grid_offset.x, state.grid_cells.x) : 0);
  const auto sy = (state.grid_cells.y > 0 ? 
    div_ceil(sheet->height() - state.grid_offset.y, state.grid_cells.y) : 0);
  state.grid.x = (sx ? sx : sy);
  state.grid.y = (sy ? sy : sx);
  check(state.grid.x > 0 && state.grid.y > 0, "invalid grid");
}

void InputParser::deduce_grid_sprites(State& state) {
  deduce_grid_size(state);
  const auto sheet = get_sheet(state);

  auto stride = state.grid;
  stride.x += state.grid_spacing.x;
  stride.y += state.grid_spacing.y;

  auto bounds = sheet->bounds();
  bounds.x += state.grid_offset.x;
  bounds.w -= state.grid_offset.x;
  bounds.y += state.grid_offset.y;
  bounds.h -= state.grid_offset.y;

  auto cells_x = state.grid_cells.x;
  auto cells_y = state.grid_cells.y;
  if (!cells_x)
    cells_x = ceil(bounds.w, stride.x) / stride.x;
  if (!cells_y)
    cells_y = ceil(bounds.h, stride.y) / stride.y;

  for (auto y = 0; y < cells_y; ++y) {
    auto output_offset = false;
    auto skipped = 0;
    for (auto x = 0; x < cells_x; ++x) {

      state.rect = intersect({
        bounds.x + x * stride.x,
        bounds.y + y * stride.y,
        state.grid.x, state.grid.y
      }, bounds);

      if (empty(state.rect))
        continue;

      if (state.trim_gray_levels ?
          is_fully_black(*sheet, state.trim_threshold, state.rect) :
          is_fully_transparent(*sheet, state.trim_threshold, state.rect)) {
        ++skipped;
        continue;
      }

      if (m_settings.autocomplete) {
        auto& os = m_autocomplete_output;
        if (!std::exchange(output_offset, true)) {
          if (y)
            os << state.indent << "row " << y << "\n";
        }

        if (skipped > 0) {
          os << state.indent << m_detected_indentation << "skip";
          if (skipped > 1)
            os << " " << skipped;
          os << "\n";
          skipped = 0;
        }

        os << state.indent << m_detected_indentation << "sprite\n";
      }

      sprite_ends(state);
    }
  }
}

void InputParser::deduce_atlas_sprites(State& state) {
  const auto sheet = get_sheet(state);
  for (const auto& rect : find_islands(*sheet,
      state.atlas_merge_distance, state.trim_gray_levels)) {
    if (m_settings.autocomplete) {
      auto& os = m_autocomplete_output;
      os << state.indent << "sprite \n";
      if (rect != sheet->bounds())
        os << state.indent << "  rect "
          << rect.x << " " << rect.y << " "
          << rect.w << " " << rect.h << "\n";
    }
    state.rect = rect;
    sprite_ends(state);
  }
}

void InputParser::deduce_single_sprite(State& state) {
  const auto sheet = get_sheet(state);
  state.rect = sheet->bounds();
  sprite_ends(state);
}

void InputParser::output_ends(State& state) {
  get_output(state);
}

void InputParser::sheet_ends(State& state) {
  update_applied_definitions(Definition::input);
  if (!m_sprites_in_current_sheet) {
    if (is_globbing_pattern(state.sheet.filename())) {
      deduce_globbing_sheets(state);
    }
    else if (state.sheet.is_sequence()) {
      deduce_sequence_sprites(state);
    }
    else if (has_grid(state)) {
      deduce_grid_sprites(state);
    }
    else if (state.atlas_merge_distance >= 0) {
      deduce_atlas_sprites(state);
    }
    else {
      deduce_single_sprite(state);
    }
  }
  m_sprites_in_current_sheet = { };
  m_current_sequence_index = { };
}

void InputParser::scope_ends(State& state) {
  switch (state.definition) {
    case Definition::output:
      output_ends(state);
      break;
    case Definition::input:
      sheet_ends(state);
      break;
    case Definition::sprite:
      sprite_ends(state);
      break;
    default:
      break;
  }
}

InputParser::InputParser(const Settings& settings)
  : m_settings(settings) {
}

void InputParser::parse(std::istream& input, 
    const std::filesystem::path& input_file) try {
  m_autocomplete_output = { };
  m_detected_indentation = "  ";
  m_input_file = input_file;

  auto indentation_detected = false;
  auto scope_stack = std::vector<State>();
  auto& top = scope_stack.emplace_back();
  top.level = -1;
  top.output = default_output_name;
  top.sprite_id = default_sprite_id;

  auto autocomplete_space = std::stringstream();
  const auto pop_scope_stack = [&](int level) {
    for (auto last = scope_stack.rbegin(); ; ++last) {
      if (has_implicit_scope(last->definition) && level <= last->level) {
        auto& state = scope_stack.back();
        state.definition = last->definition;

        // add indentation before autocompleting in implicit scope
        if (&*last == &scope_stack.back())
          state.indent += m_detected_indentation;

        scope_ends(state);
      }
      else if (level >= last->level) {
        const auto top = last.base();

        // keep texture set on same level
        if (top != scope_stack.end() &&
            top != scope_stack.begin() &&
            top->definition == Definition::output)
          std::prev(top)->output = top->output;

        for (auto i = 0u; i < std::distance(top, scope_stack.end()); ++i) {
          check_not_applied_definitions();
          m_not_applied_definitions.pop_back();
        }
        scope_stack.erase(top, scope_stack.end());
        return;
      }
    }
  };

  auto buffer = std::string();
  auto arguments = std::vector<std::string_view>();
  for (m_line_number = 1; !input.eof(); ++m_line_number) {
    std::getline(input, buffer);

    // skip UTF-8 BOM
    if (m_line_number == 1 && starts_with(buffer.data(), "\xEF\xBB\xBF")) {
      --m_line_number;
      continue;
    }

    auto line = ltrim(buffer);
    const auto level = to_int(buffer.size() - line.size());

    if (auto hash = line.find('#'); hash != std::string::npos)
      line = line.substr(0, hash);

    if (line.empty()) {
      if (m_settings.autocomplete) {
        if (input.eof())
          m_autocomplete_output << autocomplete_space.str();
        else
          autocomplete_space << buffer << '\n';
      }
      continue;
    }

    split_arguments(line, &arguments);
    const auto definition = get_definition(arguments[0]);
    if (definition == Definition::none)
      error("invalid definition '", arguments[0], "'");
    arguments.erase(arguments.begin());

    pop_scope_stack(level);

    if (level > scope_stack.back().level || has_implicit_scope(definition)) {
      scope_stack.push_back(scope_stack.back());
      m_not_applied_definitions.emplace_back();
    }

    auto& state = scope_stack.back();
    state.definition = definition;
    state.level = level;
    state.indent = std::string(begin(buffer), begin(buffer) + level);

    if (!indentation_detected && !state.indent.empty()) {
      m_detected_indentation = state.indent;
      indentation_detected = true;
    }

    apply_definition(definition, arguments,
        state, m_current_grid_cell, m_variables);
    update_not_applied_definitions(definition);

    if (m_settings.autocomplete) {
      const auto space = autocomplete_space.str();
      m_autocomplete_output << space << buffer << '\n';
      autocomplete_space = { };
    }
  }
  m_line_number = 0;
  pop_scope_stack(-1);
  assert(m_not_applied_definitions.empty());
}
catch (const std::exception& ex) {
  auto ss = std::stringstream();
  if (!m_input_file.empty())
    ss << "'" << path_to_utf8(m_input_file) << "' ";
  ss << ex.what();
  if (m_line_number > 0)
    ss << " in line " << m_line_number;
  throw std::runtime_error(ss.str());
}

void InputParser::update_applied_definitions(Definition definition) {
  for (auto& not_applied_definitions : m_not_applied_definitions)
    not_applied_definitions.erase(definition);
}

void InputParser::update_not_applied_definitions(Definition definition) {
  // outputs can also be explicitly assigned, tolerate when it is not at all
  if (definition == Definition::output)
    return;

  assert(!m_not_applied_definitions.empty());
  if (auto affected = get_affected_definition(definition); affected != Definition::none)
    m_not_applied_definitions.back().try_emplace(affected, 
      NotAppliedDefinition{ definition, m_line_number });
}

void InputParser::check_not_applied_definitions() {
  assert(!m_not_applied_definitions.empty());
  for (const auto& [affected, not_applied] : m_not_applied_definitions.back()) {
    m_line_number = not_applied.line_number;
    error("no ", get_definition_name(affected), " is affected by ", 
      get_definition_name(not_applied.definition));
  }
}

} // namespace
