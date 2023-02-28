
#include "InputParser.h"
#include "globbing.h"
#include <charconv>
#include <algorithm>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  const auto default_sprite_id = "sprite_{{ index }}";

  ImagePtr try_get_map(const ImagePtr& source, 
      const std::string& default_map_suffix, 
      const std::string& map_suffix) {

    auto map_filename = replace_suffix(source->filename(), 
      default_map_suffix, map_suffix);

    if (std::filesystem::exists(map_filename))
      return std::make_shared<Image>(source->path(), map_filename);
    return { };
  }

  bool has_map_suffix(const std::string& filename,
      const std::vector<std::string>& map_suffixes) {
    for (const auto& suffix : map_suffixes)
      if (has_suffix(filename, suffix))
        return true;
    return false;
  }

  bool has_implicit_scope(Definition definition) {
    return (definition == Definition::sheet ||
            definition == Definition::output ||
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

std::shared_ptr<Sheet> InputParser::get_sheet(const std::string& sheet_id) {
  check(!sheet_id.empty(), "no sheet specified");
  auto& sheet = m_sheets[sheet_id];
  if (!sheet)
    sheet = std::make_shared<Sheet>();
  return sheet;
}

std::shared_ptr<Output> InputParser::get_output(
    const std::filesystem::path& filename) {
  auto& output = m_outputs[std::filesystem::weakly_canonical(filename)];
  if (!output)
    output = std::make_shared<Output>();
  return output;
}

ImagePtr InputParser::get_source(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey) {
  auto& source = m_sources[std::filesystem::weakly_canonical(path / filename)];
  if (!source) {
    auto image = Image(path, filename);

    if (colorkey != RGBA{ }) {
      if (!colorkey.a)
        colorkey = guess_colorkey(image);
      replace_color(image, colorkey, RGBA{ });
    }
    source = std::make_shared<Image>(std::move(image));
  }
  return source;
}

ImagePtr InputParser::get_source(const State& state, int index) {
  return get_source(state.path,
    utf8_to_path(state.source_filenames.get_nth_filename(index)),
    state.colorkey);
}

ImagePtr InputParser::get_source(const State& state) {
  return get_source(state, m_current_sequence_index);
}

MapVectorPtr InputParser::get_maps(const State& state, const ImagePtr& source) {
  if (state.map_suffixes.empty())
    return { };

  auto it = m_maps.find(source);
  if (it == m_maps.end()) {
    auto maps = std::vector<ImagePtr>();
    for (const auto& map_suffix : state.map_suffixes)
      maps.push_back(try_get_map(source, 
        state.default_map_suffix, map_suffix));
    it = m_maps.emplace(source, 
      std::make_shared<decltype(maps)>(std::move(maps))).first;
  }
  return it->second;
}

void InputParser::sprite_ends(State& state) {
  check(!state.source_filenames.empty(), "sprite not on input");
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
  sprite.input_sprite_index = m_sprites_in_current_source;
  sprite.id = state.sprite_id;
  sprite.sheet = get_sheet(state.sheet_id);
  sprite.source = get_source(state);
  sprite.maps = get_maps(state, sprite.source);
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
  sprite.divisible_size = state.divisible_size;
  sprite.tags = state.tags;
  sprite.data = state.data;
  validate_sprite(sprite);
  m_sprites.push_back(std::move(sprite));

  if (state.source_filenames.is_sequence())
    ++m_current_sequence_index;
  ++m_sprites_in_current_source;
}

void InputParser::deduce_globbing_sources(State& state) {
  for (const auto& sequence : glob_sequences(state.path, state.source_filenames.filename())) {
    if (has_map_suffix(sequence.filename(), state.map_suffixes))
      continue;

    state.source_filenames = sequence;
    source_ends(state);
  }
}

void InputParser::deduce_sequence_sprites(State& state) {
  auto error = std::error_code{ };
  if (state.source_filenames.is_infinite_sequence()) {
    for (auto i = 0; ; ++i)
      if (!std::filesystem::exists(state.path / state.source_filenames.get_nth_filename(i), error)) {
        state.source_filenames.set_count(i);
        break;
      }
    if (state.source_filenames.is_infinite_sequence())
      return;
  }

  for (auto i = 0; i < state.source_filenames.count(); ++i) {
    const auto source = get_source(state, i);
    state.rect = source->bounds();
    sprite_ends(state);
  }
}

void InputParser::deduce_grid_size(State& state) {
  if (!empty(state.grid) ||
      empty(state.grid_cells))
    return;

  const auto source = get_source(state);
  const auto sx = (state.grid_cells.x > 0 ? 
    div_ceil(source->width() - state.grid_offset.x, state.grid_cells.x) : 0);
  const auto sy = (state.grid_cells.y > 0 ? 
    div_ceil(source->height() - state.grid_offset.y, state.grid_cells.y) : 0);
  state.grid.x = (sx ? sx : sy);
  state.grid.y = (sy ? sy : sx);
  check(state.grid.x > 0 && state.grid.y > 0, "invalid grid");
}

void InputParser::deduce_grid_sprites(State& state) {
  deduce_grid_size(state);
  const auto source = get_source(state);

  auto stride = state.grid;
  stride.x += state.grid_spacing.x;
  stride.y += state.grid_spacing.y;

  auto bounds = source->bounds();
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
          is_fully_black(*source, state.trim_threshold, state.rect) :
          is_fully_transparent(*source, state.trim_threshold, state.rect)) {
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
  const auto source = get_source(state);
  for (const auto& rect : find_islands(*source,
      state.atlas_merge_distance, state.trim_gray_levels)) {
    if (m_settings.autocomplete) {
      auto& os = m_autocomplete_output;
      os << state.indent << "sprite \n";
      if (rect != source->bounds())
        os << state.indent << "  rect "
          << rect.x << " " << rect.y << " "
          << rect.w << " " << rect.h << "\n";
    }
    state.rect = rect;
    sprite_ends(state);
  }
}

void InputParser::deduce_single_sprite(State& state) {
  const auto source = get_source(state);
  state.rect = source->bounds();
  sprite_ends(state);
}

void InputParser::sheet_ends(State& state) {
  // sheet can be opened multiple times, copy state only the first time
  auto& sheet = *get_sheet(state.sheet_id);
  if (!sheet.id.empty())
    return;

  update_applied_definitions(Definition::sheet);
  for (const auto& filename : state.output_filenames)
    sheet.outputs.push_back(get_output(filename));
  sheet.index = to_int(m_sheets.size() - 1);
  sheet.id = state.sheet_id;
  sheet.input_file = m_input_file;
  sheet.width = state.width;
  sheet.height = state.height;
  sheet.max_width = state.max_width;
  sheet.max_height = state.max_height;
  sheet.power_of_two = state.power_of_two;
  sheet.square = state.square;
  sheet.divisible_width = state.divisible_width;
  sheet.allow_rotate = state.allow_rotate;
  sheet.border_padding = state.border_padding;
  sheet.shape_padding = state.shape_padding;
  sheet.duplicates = state.duplicates;
  sheet.pack = state.pack;
}

void InputParser::output_ends(State& state) {
  update_applied_definitions(Definition::output);
  const auto& filename = state.output_filenames.back();
  auto output = get_output(filename);
  output->filename = FilenameSequence(path_to_utf8(filename));
  output->default_map_suffix = state.default_map_suffix;
  output->map_suffixes = state.map_suffixes;
  output->alpha = state.alpha;
  output->colorkey = state.alpha_colorkey;
  output->scale = state.scale;
  output->scale_filter = state.scale_filter;
}

void InputParser::source_ends(State& state) {
  update_applied_definitions(Definition::input);
  if (!m_sprites_in_current_source) {
    if (is_globbing_pattern(state.source_filenames.filename())) {
      deduce_globbing_sources(state);
    }
    else if (state.source_filenames.is_sequence()) {
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
  m_sprites_in_current_source = { };
  m_current_sequence_index = { };
}

void InputParser::scope_ends(State& state) {
  switch (state.definition) {
    case Definition::sheet:
      sheet_ends(state);
      break;
    case Definition::output:
      output_ends(state);
      break;
    case Definition::input:
      source_ends(state);
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
  auto top = &scope_stack.emplace_back();
  top->level = -1;
  top->sprite_id = default_sprite_id;
  m_not_applied_definitions.emplace_back();

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

        // sheets and outputs open a scope and affect parent scope
        // keep id/filename set when popping their scope
        if (top != scope_stack.end() && top != scope_stack.begin()) {
          if (top->definition == Definition::sheet)
            std::prev(top)->sheet_id = std::move(top->sheet_id);
          else if (top->definition == Definition::output)
            std::prev(top)->output_filenames = std::move(top->output_filenames);
        }

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
  check_not_applied_definitions();
  m_not_applied_definitions.pop_back();
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
  assert(!m_not_applied_definitions.empty());
  if (auto affected = get_affected_definition(definition); affected != Definition::none) {
    auto it = std::prev(m_not_applied_definitions.end());
    if (has_implicit_scope(definition))
      it = std::prev(it);
    it->try_emplace(affected, NotAppliedDefinition{ definition, m_line_number });
  }
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
