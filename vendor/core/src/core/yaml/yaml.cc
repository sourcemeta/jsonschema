// See https://pyyaml.org/wiki/LibYAML for basic documentation
#include <yaml.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/yaml.h>

#include <sstream>       // std::ostringstream, std::istringstream
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <tuple>         // std::tuple
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

namespace {

auto interpret_scalar(const std::string_view input,
                      const yaml_scalar_style_t style)
    -> sourcemeta::core::JSON {
  if (style == YAML_SINGLE_QUOTED_SCALAR_STYLE ||
      style == YAML_DOUBLE_QUOTED_SCALAR_STYLE ||
      style == YAML_LITERAL_SCALAR_STYLE || style == YAML_FOLDED_SCALAR_STYLE) {
    return sourcemeta::core::JSON{input};
  }

  if (input.empty()) {
    return sourcemeta::core::JSON{nullptr};
  }

  if (input.size() > 2 && input[0] == '0' &&
      (input[1] == 'x' || input[1] == 'X')) {
    try {
      const std::string hex_string{input.substr(2)};
      const std::size_t hex_value{std::stoull(hex_string, nullptr, 16)};
      return sourcemeta::core::JSON{static_cast<std::int64_t>(hex_value)};
      // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const std::invalid_argument &) {
      // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (const std::out_of_range &) {
    }
  }

  std::istringstream stream{std::string{input}};

  try {
    auto result{sourcemeta::core::parse_json(stream)};

    if (stream.peek() != std::char_traits<char>::eof()) {
      return sourcemeta::core::JSON{input};
    }

    return result;
  } catch (const sourcemeta::core::JSONParseError &) {
    return sourcemeta::core::JSON{input};
  }
}

struct AnchoredValue {
  sourcemeta::core::JSON value;
  std::vector<std::tuple<sourcemeta::core::JSON::ParsePhase,
                         sourcemeta::core::JSON::Type, std::uint64_t,
                         std::uint64_t, sourcemeta::core::JSON::ParseContext,
                         std::size_t, std::string>>
      callbacks;
};

using AnchorMap = std::unordered_map<std::string, AnchoredValue>;

auto consume_value_event(yaml_parser_t *parser, yaml_event_t *event,
                         const sourcemeta::core::JSON::ParseCallback &callback,
                         const sourcemeta::core::JSON::ParseContext context,
                         const std::size_t index,
                         const sourcemeta::core::JSON::StringView property,
                         AnchorMap &anchors,
                         const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON;

auto consume_scalar_event(yaml_event_t *event,
                          const sourcemeta::core::JSON::ParseCallback &callback,
                          const sourcemeta::core::JSON::ParseContext context,
                          const std::size_t index,
                          const sourcemeta::core::JSON::StringView property,
                          AnchorMap &anchors,
                          const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON {
  const std::string_view input{
      reinterpret_cast<char *>(event->data.scalar.value),
      event->data.scalar.length};

  const yaml_mark_t &start_mark{override_start_mark ? *override_start_mark
                                                    : event->start_mark};
  const std::uint64_t start_line{start_mark.line + 1};
  const std::uint64_t start_column{start_mark.column + 1};
  const std::uint64_t end_line{event->end_mark.line + 1};
  const std::uint64_t end_column{event->end_mark.column};

  sourcemeta::core::JSON result{nullptr};
  if (event->data.scalar.tag) {
    const std::string_view tag{
        reinterpret_cast<char *>(event->data.scalar.tag)};
    if (tag == "!" || tag == "tag:yaml.org,2002:str" || tag == "!!str") {
      result = sourcemeta::core::JSON{input};
    } else {
      result = interpret_scalar(input, event->data.scalar.style);
    }
  } else {
    result = interpret_scalar(input, event->data.scalar.style);
  }

  sourcemeta::core::JSON::Type type{sourcemeta::core::JSON::Type::Null};
  if (result.is_boolean()) {
    type = sourcemeta::core::JSON::Type::Boolean;
  } else if (result.is_integer()) {
    type = sourcemeta::core::JSON::Type::Integer;
  } else if (result.is_real()) {
    type = sourcemeta::core::JSON::Type::Real;
  } else if (result.is_decimal()) {
    type = sourcemeta::core::JSON::Type::Decimal;
  } else if (result.is_string()) {
    type = sourcemeta::core::JSON::Type::String;
  }

  std::vector<std::tuple<sourcemeta::core::JSON::ParsePhase,
                         sourcemeta::core::JSON::Type, std::uint64_t,
                         std::uint64_t, sourcemeta::core::JSON::ParseContext,
                         std::size_t, std::string>>
      recorded_callbacks;

  if (callback) {
    callback(sourcemeta::core::JSON::ParsePhase::Pre, type, start_line,
             start_column, context, index, property);
    recorded_callbacks.emplace_back(sourcemeta::core::JSON::ParsePhase::Pre,
                                    type, start_line, start_column, context,
                                    index, std::string{property});
    callback(sourcemeta::core::JSON::ParsePhase::Post, type, end_line,
             end_column, sourcemeta::core::JSON::ParseContext::Root, 0,
             sourcemeta::core::JSON::StringView{});
    recorded_callbacks.emplace_back(
        sourcemeta::core::JSON::ParsePhase::Post, type, end_line, end_column,
        sourcemeta::core::JSON::ParseContext::Root, 0, "");
  }

  if (event->data.scalar.anchor) {
    const std::string anchor_name{
        reinterpret_cast<char *>(event->data.scalar.anchor)};
    anchors.insert_or_assign(
        anchor_name, AnchoredValue{.value = result,
                                   .callbacks = std::move(recorded_callbacks)});
  }

  return result;
}

auto consume_sequence_events(
    yaml_parser_t *parser, yaml_event_t *start_event,
    const sourcemeta::core::JSON::ParseCallback &callback,
    const sourcemeta::core::JSON::ParseContext context,
    const std::size_t context_index,
    const sourcemeta::core::JSON::StringView context_property,
    AnchorMap &anchors, const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON {
  const yaml_mark_t &start_mark{override_start_mark ? *override_start_mark
                                                    : start_event->start_mark};
  const std::uint64_t start_line{start_mark.line + 1};
  const std::uint64_t start_column{start_mark.column + 1};

  const bool has_anchor{start_event->data.sequence_start.anchor != nullptr};
  std::vector<std::tuple<sourcemeta::core::JSON::ParsePhase,
                         sourcemeta::core::JSON::Type, std::uint64_t,
                         std::uint64_t, sourcemeta::core::JSON::ParseContext,
                         std::size_t, std::string>>
      recorded_callbacks;

  sourcemeta::core::JSON::ParseCallback effective_callback;
  if (has_anchor && callback) {
    effective_callback = [&](const sourcemeta::core::JSON::ParsePhase phase,
                             const sourcemeta::core::JSON::Type type,
                             const std::uint64_t line,
                             const std::uint64_t column,
                             const sourcemeta::core::JSON::ParseContext ctx,
                             const std::size_t idx,
                             const sourcemeta::core::JSON::StringView prop) {
      recorded_callbacks.emplace_back(phase, type, line, column, ctx, idx,
                                      std::string{prop});
      callback(phase, type, line, column, ctx, idx, prop);
    };
  } else {
    effective_callback = callback;
  }

  if (effective_callback) {
    effective_callback(sourcemeta::core::JSON::ParsePhase::Pre,
                       sourcemeta::core::JSON::Type::Array, start_line,
                       start_column, context, context_index, context_property);
  }

  auto result{sourcemeta::core::JSON::make_array()};
  std::size_t index{0};

  while (true) {
    yaml_event_t event;
    if (!yaml_parser_parse(parser, &event)) {
      throw sourcemeta::core::YAMLParseError("Failed to parse YAML event");
    }

    if (event.type == YAML_SEQUENCE_END_EVENT) {
      const std::uint64_t end_line{event.end_mark.line + 1};
      const std::uint64_t end_column{event.end_mark.column};
      yaml_event_delete(&event);

      if (effective_callback) {
        effective_callback(sourcemeta::core::JSON::ParsePhase::Post,
                           sourcemeta::core::JSON::Type::Array, end_line,
                           end_column,
                           sourcemeta::core::JSON::ParseContext::Root, 0,
                           sourcemeta::core::JSON::StringView{});
      }

      if (has_anchor) {
        const std::string anchor_name{
            reinterpret_cast<char *>(start_event->data.sequence_start.anchor)};
        anchors.insert_or_assign(
            anchor_name,
            AnchoredValue{.value = result,
                          .callbacks = std::move(recorded_callbacks)});
      }

      return result;
    }

    auto value{consume_value_event(parser, &event, effective_callback,
                                   sourcemeta::core::JSON::ParseContext::Index,
                                   index, sourcemeta::core::JSON::StringView{},
                                   anchors)};
    result.push_back(std::move(value));
    index++;
  }
}

auto consume_mapping_events(
    yaml_parser_t *parser, yaml_event_t *start_event,
    const sourcemeta::core::JSON::ParseCallback &callback,
    const sourcemeta::core::JSON::ParseContext context,
    const std::size_t context_index,
    const sourcemeta::core::JSON::StringView context_property,
    AnchorMap &anchors, const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON {
  const yaml_mark_t &start_mark{override_start_mark ? *override_start_mark
                                                    : start_event->start_mark};
  const std::uint64_t start_line{start_mark.line + 1};
  const std::uint64_t start_column{start_mark.column + 1};

  const bool has_anchor{start_event->data.mapping_start.anchor != nullptr};
  std::vector<std::tuple<sourcemeta::core::JSON::ParsePhase,
                         sourcemeta::core::JSON::Type, std::uint64_t,
                         std::uint64_t, sourcemeta::core::JSON::ParseContext,
                         std::size_t, std::string>>
      recorded_callbacks;

  sourcemeta::core::JSON::ParseCallback effective_callback;
  if (has_anchor && callback) {
    effective_callback = [&](const sourcemeta::core::JSON::ParsePhase phase,
                             const sourcemeta::core::JSON::Type type,
                             const std::uint64_t line,
                             const std::uint64_t column,
                             const sourcemeta::core::JSON::ParseContext ctx,
                             const std::size_t idx,
                             const sourcemeta::core::JSON::StringView prop) {
      recorded_callbacks.emplace_back(phase, type, line, column, ctx, idx,
                                      std::string{prop});
      callback(phase, type, line, column, ctx, idx, prop);
    };
  } else {
    effective_callback = callback;
  }

  if (effective_callback) {
    effective_callback(sourcemeta::core::JSON::ParsePhase::Pre,
                       sourcemeta::core::JSON::Type::Object, start_line,
                       start_column, context, context_index, context_property);
  }

  auto result{sourcemeta::core::JSON::make_object()};

  while (true) {
    yaml_event_t key_event;
    if (!yaml_parser_parse(parser, &key_event)) {
      throw sourcemeta::core::YAMLParseError("Failed to parse YAML event");
    }

    if (key_event.type == YAML_MAPPING_END_EVENT) {
      const std::uint64_t end_line{key_event.end_mark.line + 1};
      const std::uint64_t end_column{key_event.end_mark.column};
      yaml_event_delete(&key_event);

      if (effective_callback) {
        effective_callback(sourcemeta::core::JSON::ParsePhase::Post,
                           sourcemeta::core::JSON::Type::Object, end_line,
                           end_column,
                           sourcemeta::core::JSON::ParseContext::Root, 0,
                           sourcemeta::core::JSON::StringView{});
      }

      if (has_anchor) {
        const std::string anchor_name{
            reinterpret_cast<char *>(start_event->data.mapping_start.anchor)};
        anchors.insert_or_assign(
            anchor_name,
            AnchoredValue{.value = result,
                          .callbacks = std::move(recorded_callbacks)});
      }

      return result;
    }

    if (key_event.type != YAML_SCALAR_EVENT) {
      yaml_event_delete(&key_event);
      throw sourcemeta::core::YAMLParseError(
          "Expected scalar key in YAML mapping");
    }

    const std::string key{reinterpret_cast<char *>(key_event.data.scalar.value),
                          key_event.data.scalar.length};
    const yaml_mark_t key_start_mark{key_event.start_mark};
    yaml_event_delete(&key_event);

    yaml_event_t value_event;
    if (!yaml_parser_parse(parser, &value_event)) {
      throw sourcemeta::core::YAMLParseError("Failed to parse YAML event");
    }

    auto value{
        consume_value_event(parser, &value_event, effective_callback,
                            sourcemeta::core::JSON::ParseContext::Property, 0,
                            key, anchors, &key_start_mark)};
    result.assign(key, std::move(value));
  }
}

auto consume_value_event(yaml_parser_t *parser, yaml_event_t *event,
                         const sourcemeta::core::JSON::ParseCallback &callback,
                         const sourcemeta::core::JSON::ParseContext context,
                         const std::size_t index,
                         const sourcemeta::core::JSON::StringView property,
                         AnchorMap &anchors,
                         const yaml_mark_t *override_start_mark)
    -> sourcemeta::core::JSON {
  sourcemeta::core::JSON result{nullptr};

  switch (event->type) {
    case YAML_SCALAR_EVENT:
      result = consume_scalar_event(event, callback, context, index, property,
                                    anchors, override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_SEQUENCE_START_EVENT:
      result = consume_sequence_events(parser, event, callback, context, index,
                                       property, anchors, override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_MAPPING_START_EVENT:
      result = consume_mapping_events(parser, event, callback, context, index,
                                      property, anchors, override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_ALIAS_EVENT: {
      const std::string anchor_name{
          reinterpret_cast<char *>(event->data.alias.anchor)};
      const std::uint64_t alias_start_line{(override_start_mark
                                                ? override_start_mark->line
                                                : event->start_mark.line) +
                                           1};
      const std::uint64_t alias_start_column{(override_start_mark
                                                  ? override_start_mark->column
                                                  : event->start_mark.column) +
                                             1};
      const std::uint64_t alias_end_line{event->end_mark.line + 1};
      const std::uint64_t alias_end_column{event->end_mark.column};
      yaml_event_delete(event);

      const auto anchor_it{anchors.find(anchor_name)};
      if (anchor_it == anchors.end()) {
        throw sourcemeta::core::YAMLUnknownAnchorError(anchor_name);
      }

      result = anchor_it->second.value;

      if (callback) {
        const auto &callbacks{anchor_it->second.callbacks};
        if (!callbacks.empty()) {
          const auto value_type{std::get<1>(callbacks.front())};
          callback(sourcemeta::core::JSON::ParsePhase::Pre, value_type,
                   alias_start_line, alias_start_column, context, index,
                   property);

          if (result.is_object() || result.is_array()) {
            for (std::size_t callback_index = 1;
                 callback_index < callbacks.size() - 1; callback_index++) {
              callback(std::get<0>(callbacks[callback_index]),
                       std::get<1>(callbacks[callback_index]),
                       std::get<2>(callbacks[callback_index]),
                       std::get<3>(callbacks[callback_index]),
                       std::get<4>(callbacks[callback_index]),
                       std::get<5>(callbacks[callback_index]),
                       std::get<6>(callbacks[callback_index]));
            }
          }

          callback(sourcemeta::core::JSON::ParsePhase::Post, value_type,
                   alias_end_line, alias_end_column,
                   sourcemeta::core::JSON::ParseContext::Root, 0,
                   sourcemeta::core::JSON::StringView{});
        }
      }

      break;
    }

    default:
      yaml_event_delete(event);
      throw sourcemeta::core::YAMLParseError("Unexpected YAML event type");
  }

  return result;
}

auto parse_yaml_from_events(
    yaml_parser_t *parser,
    const sourcemeta::core::JSON::ParseCallback &callback)
    -> sourcemeta::core::JSON {
  AnchorMap anchors;
  yaml_event_t event;

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML stream");
  }
  if (event.type != YAML_STREAM_START_EVENT) {
    yaml_event_delete(&event);
    throw sourcemeta::core::YAMLParseError("Expected YAML stream start");
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML document");
  }
  if (event.type != YAML_DOCUMENT_START_EVENT) {
    yaml_event_delete(&event);
    throw sourcemeta::core::YAMLParseError("Expected YAML document start");
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML value");
  }

  auto result{consume_value_event(
      parser, &event, callback, sourcemeta::core::JSON::ParseContext::Root, 0,
      sourcemeta::core::JSON::StringView{}, anchors)};

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML document end");
  }
  if (event.type != YAML_DOCUMENT_END_EVENT) {
    yaml_event_delete(&event);
    throw sourcemeta::core::YAMLParseError("Expected YAML document end");
  }
  yaml_event_delete(&event);

  return result;
}

} // namespace

namespace {

struct StreamContext {
  std::basic_istream<sourcemeta::core::JSON::Char,
                     sourcemeta::core::JSON::CharTraits> *stream;
  std::size_t total_bytes_read{0};
};

auto yaml_stream_reader(void *data, unsigned char *buffer, std::size_t size,
                        std::size_t *size_read) -> int {
  auto *context = static_cast<StreamContext *>(data);
  context->stream->read(reinterpret_cast<char *>(buffer),
                        static_cast<std::streamsize>(size));
  *size_read = static_cast<std::size_t>(context->stream->gcount());
  context->total_bytes_read += *size_read;
  return 1;
}

} // namespace

namespace sourcemeta::core {

auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback) -> JSON {
  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    throw YAMLError("Could not initialize the YAML parser");
  }

  StreamContext context{.stream = &stream, .total_bytes_read = 0};
  yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
  yaml_parser_set_input(&parser, yaml_stream_reader, &context);

  try {
    const auto result{parse_yaml_from_events(&parser, callback)};

    yaml_event_t peek_event;
    if (yaml_parser_parse(&parser, &peek_event)) {
      if (peek_event.type == YAML_DOCUMENT_START_EVENT) {
        const std::size_t next_doc_position = peek_event.start_mark.index;
        const auto bytes_to_seek_back =
            static_cast<std::streamoff>(next_doc_position) -
            static_cast<std::streamoff>(context.total_bytes_read);
        yaml_event_delete(&peek_event);
        yaml_parser_delete(&parser);
        stream.clear();
        stream.seekg(bytes_to_seek_back, std::ios::cur);
        return result;
      }
      yaml_event_delete(&peek_event);
    }

    yaml_parser_delete(&parser);
    return result;
  } catch (...) {
    yaml_parser_delete(&parser);
    throw;
  }
}

auto parse_yaml(const JSON::String &input, const JSON::ParseCallback &callback)
    -> JSON {
  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    throw YAMLError("Could not initialize the YAML parser");
  }

  yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);
  yaml_parser_set_input_string(
      &parser, reinterpret_cast<const unsigned char *>(input.c_str()),
      input.size());

  try {
    const auto result{parse_yaml_from_events(&parser, callback)};
    yaml_parser_delete(&parser);
    return result;
  } catch (...) {
    yaml_parser_delete(&parser);
    throw;
  }
}

auto read_yaml(const std::filesystem::path &path,
               const JSON::ParseCallback &callback) -> JSON {
  auto stream = read_file(path);
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return parse_yaml(buffer.str(), callback);
}

auto read_yaml_or_json(const std::filesystem::path &path,
                       const JSON::ParseCallback &callback) -> JSON {
  const auto extension{path.extension()};
  if (extension == ".yaml" || extension == ".yml") {
    return read_yaml(path, callback);
  } else if (extension == ".json") {
    return read_json(path, callback);
  }

  try {
    return read_json(path, callback);
  } catch (const JSONParseError &) {
    return read_yaml(path, callback);
  }
}

} // namespace sourcemeta::core
