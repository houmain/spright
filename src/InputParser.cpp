
#include "InputParser.h"
#include "globbing.h"
#include <charconv>
#include <algorithm>
#include <cstring>
#include <utility>

namespace spright {

namespace {
  const auto default_sheet_id = "spright";
  const auto default_sprite_id = "sprite_{{ index }}";

  ImageFilePtr try_get_map(const ImageFilePtr& source, 
      const std::string& default_map_suffix, 
      const std::string& map_suffix) {

    auto map_filename = replace_suffix(source->filename(), 
      default_map_suffix, map_suffix);

    if (std::filesystem::exists(map_filename))
      return std::make_shared<ImageFile>(source->path(), map_filename);
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
            definition == Definition::glob ||
            definition == Definition::input ||
            definition == Definition::description ||
            definition == Definition::transform ||
            definition == Definition::sprite ||
            definition == Definition::skip);
  }

  void validate_sprite(const Sprite& sprite) {
    const auto& rect = sprite.source_rect;
    const auto& bounds = sprite.source->bounds();
    const auto intersected = intersect(bounds, rect);
    if (rect != intersected) {
      auto message = std::ostringstream();
      message << "sprite outside source (";
      if (rect.x0() < bounds.x0())      message << "x: " << rect.x0() << " < " << bounds.x0();
      else if (rect.y0() < bounds.y0()) message << "y: " << rect.y0() << " < " << bounds.y0();
      else if (rect.x1() > bounds.x1()) message << "x: " << rect.x1() << " > " << bounds.x1();
      else if (rect.y1() > bounds.y1()) message << "y: " << rect.y1() << " > " << bounds.y1();
      message << ")";
      error(message.str());
    }
  }

  std::vector<ImageFilePtr> make_unique_sort(std::vector<ImageFilePtr> sources) {
    std::sort(begin(sources), end(sources));
    sources.erase(std::unique(begin(sources), end(sources)), end(sources));
    std::sort(begin(sources), end(sources),
      [](const auto& a, const auto& b) { return 
        std::tie(a->path(), a->filename()) <
        std::tie(b->path(), b->filename()); 
      });
    return sources;
  }

  Rect intersect_overlapping(const Rect& rect, const Rect& bounds) {
    const auto intersected = intersect(rect, bounds);
    return (empty(intersected) ? rect : intersected);
  }

  template<typename C, typename F>
  bool last_n_contain(const C& container, int n, F&& function) {
    assert(n <= to_int(container.size()));
    return std::find_if(std::next(container.begin(), to_int(container.size()) - n),
      container.end(), function) != container.end();
  }
} // namespace

std::shared_ptr<Sheet> InputParser::get_sheet(const std::string& sheet_id) {
  check(!sheet_id.empty(), "no sheet specified");
  auto& sheet = m_sheets[sheet_id];
  if (!sheet)
    sheet = std::make_shared<Sheet>();
  return sheet;
}

void InputParser::set_transform(const std::string& transform_id, 
    TransformPtr transform) {
  assert(!transform_id.empty());
  const auto inserted = m_transforms.emplace(transform_id, std::move(transform)).second;
  check(inserted, "transform ", transform_id, " already defined");
}

TransformPtr InputParser::get_transform(const std::string& transform_id) {
  assert(!transform_id.empty());
  const auto it = m_transforms.find(transform_id);
  return (it != m_transforms.end() ? it->second : nullptr);
}

std::shared_ptr<Output> InputParser::get_output(
    const std::filesystem::path& filename) {
  auto& output = m_outputs[std::filesystem::weakly_canonical(filename)];
  if (!output) {
    output = std::make_shared<Output>();
    output->warning_line_number = m_warning_line_number;
  }
  return output;
}

ImageFilePtr InputParser::get_source(const std::filesystem::path& path,
    const std::filesystem::path& filename, RGBA colorkey) {
  auto& source = m_sources[std::filesystem::weakly_canonical(path / filename)];
  if (!source) {
    auto image = load_image(path / filename);

    if (colorkey != RGBA{ }) {
      if (!colorkey.a)
        colorkey = guess_colorkey(image);
      replace_color(image, colorkey, RGBA{ });
    }
    source = std::make_shared<ImageFile>(std::move(image), path, filename);
  }
  return source;
}

ImageFilePtr InputParser::get_source(const State& state, int index) {
  return get_source(state.path,
    utf8_to_path(state.source_filenames.get_nth_filename(index)),
    state.colorkey);
}

ImageFilePtr InputParser::get_source(const State& state) {
  return get_source(state, m_current_sequence_index);
}

MapVectorPtr InputParser::get_maps(const State& state, const ImageFilePtr& source) {
  if (state.map_suffixes.empty())
    return { };

  auto it = m_maps.find(source);
  if (it == m_maps.end()) {
    auto maps = std::vector<ImageFilePtr>();
    for (const auto& map_suffix : state.map_suffixes)
      maps.push_back(try_get_map(source, 
        state.default_map_suffix, map_suffix));
    it = m_maps.emplace(source, 
      std::make_shared<decltype(maps)>(std::move(maps))).first;
  }
  return it->second;
}

bool InputParser::should_autocomplete(const std::string& filename, bool is_update) const {
  if (m_settings.mode == Mode::autocomplete) {
    // complete everything matching pattern
    return (m_settings.autocomplete_pattern.empty() ||
      match(m_settings.autocomplete_pattern, filename));
  }
  else {
    // automatically deduce definitions when there are none yet
    return !is_update;
  }
}

void InputParser::sprite_ends(State& state) {
  update_applied_definitions(Definition::sprite);
  if (!state.transforms.empty())
    update_applied_definitions(Definition::transform);

  const auto source = get_source(state);
  const auto rect =
    (!empty(state.rect) ? state.rect :
     has_grid(state) ? intersect_overlapping(
       deduce_rect_from_grid(state), source->bounds()) :
     source->bounds());

  const auto advance = [&]() {
    check(m_current_sequence_index < state.source_filenames.count(), 
      "too many sprites in sequence");
    m_current_grid_cell.x += state.span.x;
    if (state.source_filenames.is_sequence())
      ++m_current_sequence_index;
  };

  if (state.skip_sprites > 0) {
    m_skipped_in_current_input.push_back(rect);
    advance();
    return;
  }

  auto sprite = Sprite{ };
  sprite.warning_line_number = m_warning_line_number;
  sprite.index = to_int(m_sprites.size());
  sprite.input_index = to_int(m_inputs.size());
  sprite.input_sprite_index = m_sprites_in_current_input;
  sprite.id = state.sprite_id;
  sprite.sheet = get_sheet(state.sheet_id);
  sprite.source = source;
  sprite.maps = get_maps(state, sprite.source);
  sprite.source_rect = rect;
  sprite.pivot = state.pivot;
  sprite.trim = state.trim;
  sprite.trim_margin = state.trim_margin;
  sprite.trim_threshold = state.trim_threshold;
  sprite.trim_gray_levels = state.trim_gray_levels;
  sprite.crop = state.crop;
  sprite.crop_pivot = state.crop_pivot;
  sprite.min_bounds = state.min_bounds;
  sprite.divisible_bounds = state.divisible_bounds;
  sprite.common_bounds = state.common_bounds;
  sprite.align = state.align;
  sprite.align_pivot = state.align_pivot;
  sprite.transforms = state.transforms;
  sprite.tags = state.tags;
  sprite.data = state.data;
  advance();

  if (state.max_sprites > 0 && 
      m_sprites_in_current_input >= state.max_sprites)
    error("max-sprites exceeded (", state.max_sprites, ")");

  validate_sprite(sprite);
  m_current_input_sources.push_back(sprite.source);
  m_sprites.push_back(std::move(sprite));
  ++m_sprites_in_current_input;
}

void InputParser::skip_sprites(State& state) {
  for (; state.skip_sprites > 0; --state.skip_sprites)
    sprite_ends(state);
}

bool InputParser::overlaps_sprite_or_skipped_rect(const Rect& rect) const {
  if (last_n_contain(m_sprites, m_sprites_in_current_input,
        [&](const Sprite& sprite) {
          return overlapping(sprite.source_rect, rect);
        }))
    return true;

  return std::find_if(m_skipped_in_current_input.begin(),
            m_skipped_in_current_input.end(),
            [&](const Rect& skipped) {
              return overlapping(skipped, rect);
            }) != m_skipped_in_current_input.end();
}

void InputParser::deduce_sequence_sprites(State& state) {
  // prevent grid and atlas in sequences
  state.grid = { };
  state.grid_cells = { };
  state.atlas_merge_distance = -1;

  auto skip_already_added = sprites_or_skips_in_current_input();
  for (auto i = 0; i < state.source_filenames.count(); ++i) {
    if (skip_already_added) {
      --skip_already_added;
      continue;
    }

    auto error = std::error_code{ };
    if (state.source_filenames.is_infinite_sequence())
      if (!std::filesystem::exists(
            state.path / state.source_filenames.get_nth_filename(i), error))
        break;

    if (m_settings.mode == Mode::autocomplete) {
      auto& os = m_autocomplete_output;
      os << state.indent << "sprite \n";
    }
    sprite_ends(state);
  }
}

void InputParser::deduce_grid_size(State& state) {
  if (state.grid.x > 0 && state.grid.y > 0)
    return;

  const auto bounds = get_grid_bounds(state);
  if (state.grid_cells.x > 0 || state.grid_cells.y > 0) {
    const auto sx = (state.grid_cells.x > 0 ?
      div_ceil(bounds.w + state.grid_spacing.x, 
        state.grid_cells.x) - state.grid_spacing.x : 0);
    const auto sy = (state.grid_cells.y > 0 ?
      div_ceil(bounds.h + state.grid_spacing.y, 
        state.grid_cells.y) - state.grid_spacing.y : 0);
    state.grid.x = (sx ? sx : sy);
    state.grid.y = (sy ? sy : sx);
  }
  if (state.grid.x <= 0)
    state.grid.x = bounds.w;
  if (state.grid.y <= 0)
    state.grid.y = bounds.h;
}

Rect InputParser::deduce_rect_from_grid(State& state) {
  assert(has_grid(state));
  deduce_grid_size(state);
  return {
    state.grid_offset.x + (state.grid.x + state.grid_spacing.x) * m_current_grid_cell.x,
    state.grid_offset.y + (state.grid.y + state.grid_spacing.y) * m_current_grid_cell.y,
    state.grid.x * state.span.x,
    state.grid.y * state.span.y
  };
}

Rect InputParser::get_grid_bounds(const State& state) {
  const auto source = get_source(state);
  auto bounds = source->bounds();
  bounds.x += state.grid_offset.x;
  bounds.y += state.grid_offset.y;
  bounds.w -= state.grid_offset.x + state.grid_offset_bottom_right.x;
  bounds.h -= state.grid_offset.y + state.grid_offset_bottom_right.y;
  return bounds;
}

void InputParser::deduce_grid_sprites(State& state) {
  deduce_grid_size(state);
  const auto source = get_source(state);

  auto stride = state.grid;
  stride.x += state.grid_spacing.x;
  stride.y += state.grid_spacing.y;

  auto cells_x = state.grid_cells.x;
  auto cells_y = state.grid_cells.y;
  if (!cells_x || !cells_y) {
    const auto bounds = get_grid_bounds(state);
    if (!cells_x)  
      cells_x = ceil(bounds.w, stride.x) / stride.x;
    if (!cells_y)
      cells_y = ceil(bounds.h, stride.y) / stride.y;
  }

  auto& x = m_current_grid_cell.x;
  auto& y = m_current_grid_cell.y;

  const auto is_update = (sprites_or_skips_in_current_input() != 0);
  for (; y < cells_y; y += state.span.y) {
    auto output_offset = (x != 0);
    auto skipped = 0;
    for (; x < cells_x; x += state.span.x) {      
      const auto rect = intersect(deduce_rect_from_grid(state), source->bounds());

      if (empty(rect) ||
          (state.trim_gray_levels ?
           is_fully_black(*source, state.trim_threshold, rect) :
           is_fully_transparent(*source, state.trim_threshold, rect))) {
        ++skipped;
        continue;
      }

      if (is_update && overlaps_sprite_or_skipped_rect(rect))
        continue;

      if (m_settings.mode == Mode::autocomplete) {
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

        os << state.indent << m_detected_indentation << "sprite \n";
      }

      sprite_ends(state);
      
      // do not advance twice (could be improved)
      x -= state.span.x;
    }
    x = 0;
  }
}

void InputParser::deduce_atlas_sprites(State& state) {
  const auto source = get_source(state);
  const auto is_update = (sprites_or_skips_in_current_input() != 0);
  for (const auto& rect : find_islands(*source,
      state.atlas_merge_distance, state.trim_gray_levels)) {
    if (is_update && overlaps_sprite_or_skipped_rect(rect))
      continue;

    if (m_settings.mode == Mode::autocomplete) {
      auto& os = m_autocomplete_output;
      os << state.indent << "sprite \n";
      if (rect != source->bounds())
        os << state.indent << m_detected_indentation << "rect "
          << rect.x << " " << rect.y << " "
          << rect.w << " " << rect.h << "\n";
    }
    state.rect = rect;
    sprite_ends(state);
  }
  state.rect = { };
}

void InputParser::deduce_single_sprite(State& state) {
  if (sprites_or_skips_in_current_input())
    return;

  if (m_settings.mode == Mode::autocomplete)
    m_autocomplete_output << state.indent << "sprite \n";

  sprite_ends(state);
}

void InputParser::sheet_ends(State& state) {
  // sheet can be opened multiple times, copy state only the first time
  auto sheet_ptr = get_sheet(state.sheet_id);
  if (!sheet_ptr->id.empty())
    return;

  auto& sheet = *sheet_ptr;
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
  output->alpha_color = state.alpha_color;
  output->scale = state.scale;
  output->scale_filter = state.scale_filter;
  output->debug = state.debug;
}

void InputParser::deduce_globbed_inputs(State& state) {
  state.indent += m_detected_indentation;

  const auto sequences = [&]() {
    auto sequences = std::vector<FilenameSequence>();
    if (has_grid(state) || has_atlas(state)) {
      // do not merge sequences when grid or atlas is active
      for (const auto& filename : glob_filenames(state.path, state.glob_pattern))
        sequences.emplace_back(filename);
    }
    else {
      sequences = glob_sequences(state.path, state.glob_pattern);
      for (auto& sequence : sequences)
        sequence.set_infinite();
    }
    return sequences;
  }();

  const auto inputs_in_sheet_before = m_inputs.size();
  for (const auto& sequence : sequences) {
    if (has_map_suffix(sequence, state.map_suffixes))
      continue;

    // only add inputs not encountered before
    if (last_n_contain(m_inputs, static_cast<int>(m_inputs.size() - inputs_in_sheet_before),
        [&](const Input& input) {
          return input.source_filenames == sequence.sequence_filename();
        }))
      continue;

    if (m_settings.mode == Mode::autocomplete)
      m_autocomplete_output << "\n" << state.indent << "input \"" <<
        sequence.sequence_filename() << "\"\n";

    auto input_state = state;
    input_state.source_filenames = sequence;
    input_ends(input_state);
  }
}

void InputParser::glob_begins([[maybe_unused]] State& state) {
  m_inputs_in_current_glob = { };
}

void InputParser::glob_ends(State& state) {
  if (should_autocomplete(state.glob_pattern, m_inputs_in_current_glob > 0))
    deduce_globbed_inputs(state);
}

void InputParser::input_ends(State& state) {
  update_applied_definitions(Definition::input);
  update_applied_definitions(Definition::sprite);
  
  if (should_autocomplete(state.source_filenames.sequence_filename(),
        sprites_or_skips_in_current_input() > 0)) {
    auto sprite_indent = state.indent + m_detected_indentation;
    std::swap(state.indent, sprite_indent);
    deduce_input_sprites(state);
    std::swap(state.indent, sprite_indent);
  }

  m_inputs.push_back({
    to_int(m_inputs.size()),
    state.source_filenames.sequence_filename(),
    make_unique_sort(std::move(m_current_input_sources))
  });
  m_sprites_in_current_input = { };
  m_skipped_in_current_input.clear();
  m_current_grid_cell = { };
  m_current_sequence_index = { };
  ++m_inputs_in_current_glob;
}

void InputParser::deduce_input_sprites(State& state) {
  if (state.source_filenames.is_sequence()) {
    deduce_sequence_sprites(state);
  }
  else if (has_grid(state)) {
    deduce_grid_sprites(state);
  }
  else if (has_atlas(state)) {
    deduce_atlas_sprites(state);
  }
  else {
    deduce_single_sprite(state);
  }
}

void InputParser::description_ends(State& state) {
  update_applied_definitions(Definition::description);
  auto& description = m_descriptions.emplace_back();
  description.filename = state.description_filename;
  description.template_filename = state.template_filename;
}

void InputParser::transform_begins(State& state, State& parent_state) {
  // transforms open a scope and affect parent scope
  if (const auto transform = get_transform(state.transform_id)) {
    // apply existing transform
    state.transforms.push_back(transform);
    parent_state.transforms.push_back(transform);
  }
  else {
    // define a new transform
    m_current_transform = std::make_shared<Transform>();
    set_transform(state.transform_id, m_current_transform);
  }
}

void InputParser::transform_ends([[maybe_unused]] State& state) {
  check(m_current_transform ||
    state.transforms.back() == get_transform(state.transform_id),
    "duplicate transform definition");
  update_applied_definitions(Definition::transform);
  m_current_transform.reset();
}

void InputParser::scope_ends(State& state) {
  switch (state.definition) {
    case Definition::sheet:
      sheet_ends(state);
      break;
    case Definition::output:
      output_ends(state);
      break;
    case Definition::glob:
      glob_ends(state);
      break;
    case Definition::input:
      input_ends(state);
      break;
    case Definition::sprite:
      sprite_ends(state);
      break;
    case Definition::skip:
      skip_sprites(state);
      break;
    case Definition::description:
      description_ends(state);
      break;
    case Definition::transform:
      transform_ends(state);
      break;
    default:
      break;
  }
}

InputParser::InputParser(Settings settings)
  : m_settings(std::move(settings)) {
}

void InputParser::parse(std::istream& input, 
    const std::filesystem::path& input_file) {
  m_autocomplete_output = { };
  m_detected_indentation = "  ";
  m_input_file = input_file;

  auto indentation_detected = false;
  auto scope_stack = std::vector<State>();
  auto top = &scope_stack.emplace_back();
  top->level = -2;
  m_not_applied_definitions.emplace_back();

  // add default sheet
  top = &scope_stack.emplace_back();
  top->level = -1;
  top->definition = Definition::sheet;
  top->sheet_id = default_sheet_id;
  top->sprite_id = default_sprite_id;
  m_not_applied_definitions.emplace_back();

  auto autocomplete_space = std::ostringstream();
  const auto pop_scope_stack = [&](int level) {
    for (auto last = scope_stack.rbegin(); ; ++last) {
      if (has_implicit_scope(last->definition) && level <= last->level) {
        handle_exception([&]() {
          // call end of scope handler before
          // unwinding (just apply actual definition and indent)
          auto& state = scope_stack.back();
          state.definition = last->definition;
          state.indent = last->indent;
          scope_ends(state);
        });
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
          handle_exception([&]() {
            check_not_applied_definitions();
          });
          m_not_applied_definitions.pop_back();
        }
        scope_stack.erase(top, scope_stack.end());
        return;
      }
    }
  };

  auto buffer = std::string();
  auto arguments = std::vector<std::string_view>();
  for (auto line_number = 1; !input.eof(); ++line_number) {
    std::getline(input, buffer);

    // skip UTF-8 BOM
    if (line_number == 1 && starts_with(buffer.data(), "\xEF\xBB\xBF")) {
      --line_number;
      continue;
    }

    auto line = ltrim(buffer);
    const auto level = to_int(buffer.size() - line.size());
    line = trim_comment(line);
    line = rtrim(line);

    if (line.empty()) {
      if (m_settings.mode == Mode::autocomplete)
        if (!input.eof())
          autocomplete_space << buffer << '\n';
      continue;
    }

    pop_scope_stack(level);

    handle_exception([&]() {
      m_warning_line_number = line_number;

      split_arguments(line, &arguments);
      const auto definition = get_definition(arguments[0]);

      if (level > scope_stack.back().level || has_implicit_scope(definition)) {
        scope_stack.push_back(scope_stack.back());
        m_not_applied_definitions.emplace_back();
      }

      auto& state = scope_stack.back();
      state.definition = Definition::none;
      state.level = level;
      state.indent = std::string(begin(buffer), begin(buffer) + level);

      if (!indentation_detected && !state.indent.empty()) {
        m_detected_indentation = state.indent;
        indentation_detected = true;
      }

      if (definition == Definition::none)
        error("invalid definition '", arguments[0], "'");
      arguments.erase(arguments.begin());

      apply_definition(definition, arguments,
          state, m_current_grid_cell, m_variables,
          m_current_transform.get());
      update_not_applied_definitions(definition, line_number);

      if (definition == Definition::glob) {
        glob_begins(state);
      }
      else if (definition == Definition::transform) {
        auto& parent_state = *std::prev(scope_stack.end(), 2);
        transform_begins(state, parent_state);
      }

      // set definition when it was successfully applied
      // to disable scope_ends() on "errors as warnings"
      state.definition = definition;
    });

    if (m_settings.mode == Mode::autocomplete) {
      m_autocomplete_output << autocomplete_space.str() << buffer << '\n';
      autocomplete_space = { };
    }
  }

  pop_scope_stack(-2);

  handle_exception([&]() {
    check_not_applied_definitions();
    m_not_applied_definitions.pop_back();
    assert(m_not_applied_definitions.empty());
  });

  if (m_settings.mode == Mode::autocomplete)
    m_autocomplete_output << autocomplete_space.str();
}

int InputParser::sprites_or_skips_in_current_input() const {
  return m_sprites_in_current_input + to_int(m_skipped_in_current_input.size());
}

void InputParser::update_applied_definitions(Definition definition) {
  for (auto& not_applied_definitions : m_not_applied_definitions)
    not_applied_definitions.erase(definition);
}

void InputParser::update_not_applied_definitions(Definition definition, int line_number) {
  assert(!m_not_applied_definitions.empty());
  if (auto affected = get_affected_definition(definition); affected != Definition::none) {
    auto it = std::prev(m_not_applied_definitions.end());
    if (has_implicit_scope(definition))
      it = std::prev(it);
    it->try_emplace(affected, NotAppliedDefinition{ definition, line_number });
  }
}

void InputParser::check_not_applied_definitions() {
  assert(!m_not_applied_definitions.empty());
  for (const auto& [affected, not_applied] : m_not_applied_definitions.back()) {
    m_warning_line_number = not_applied.line_number;
    error("no ", get_definition_name(affected), " is affected by ", 
      get_definition_name(not_applied.definition));
  }
}

void InputParser::handle_exception(std::function<void()>&& function) {
  try {
    function();
  }
  catch (const std::exception& ex) {
    warning(ex.what(), m_warning_line_number);
  }
}

} // namespace
