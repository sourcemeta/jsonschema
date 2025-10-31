// See https://pyyaml.org/wiki/LibYAML for basic documentation
#include <yaml.h>

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/yaml.h>

#include <sstream>     // std::ostringstream, std::istringstream
#include <string_view> // std::string_view

namespace {

auto interpret_scalar(const std::string_view input,
                      const yaml_scalar_style_t style)
    -> sourcemeta::core::JSON {
  if (style == YAML_SINGLE_QUOTED_SCALAR_STYLE ||
      style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
    return sourcemeta::core::JSON{input};
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

auto consume_value_event(yaml_parser_t *parser, yaml_event_t *event,
                         const sourcemeta::core::JSON::ParseCallback &callback,
                         const sourcemeta::core::JSON &context,
                         const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON;

auto consume_scalar_event(yaml_event_t *event,
                          const sourcemeta::core::JSON::ParseCallback &callback,
                          const sourcemeta::core::JSON &context,
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

  auto result{interpret_scalar(input, event->data.scalar.style)};

  sourcemeta::core::JSON::Type type{sourcemeta::core::JSON::Type::Null};
  if (result.is_boolean()) {
    type = sourcemeta::core::JSON::Type::Boolean;
  } else if (result.is_integer()) {
    type = sourcemeta::core::JSON::Type::Integer;
  } else if (result.is_real()) {
    type = sourcemeta::core::JSON::Type::Real;
  } else if (result.is_string()) {
    type = sourcemeta::core::JSON::Type::String;
  }

  if (callback) {
    callback(sourcemeta::core::JSON::ParsePhase::Pre, type, start_line,
             start_column, context);
    callback(sourcemeta::core::JSON::ParsePhase::Post, type, end_line,
             end_column, result);
  }

  return result;
}

auto consume_sequence_events(
    yaml_parser_t *parser, yaml_event_t *start_event,
    const sourcemeta::core::JSON::ParseCallback &callback,
    const sourcemeta::core::JSON &context,
    const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON {
  const yaml_mark_t &start_mark{override_start_mark ? *override_start_mark
                                                    : start_event->start_mark};
  const std::uint64_t start_line{start_mark.line + 1};
  const std::uint64_t start_column{start_mark.column + 1};

  if (callback) {
    callback(sourcemeta::core::JSON::ParsePhase::Pre,
             sourcemeta::core::JSON::Type::Array, start_line, start_column,
             context);
  }

  auto result{sourcemeta::core::JSON::make_array()};
  std::uint64_t index{0};

  while (true) {
    yaml_event_t event;
    if (!yaml_parser_parse(parser, &event)) {
      throw sourcemeta::core::YAMLParseError("Failed to parse YAML event");
    }

    if (event.type == YAML_SEQUENCE_END_EVENT) {
      const std::uint64_t end_line{event.end_mark.line + 1};
      const std::uint64_t end_column{event.end_mark.column};
      yaml_event_delete(&event);

      if (callback) {
        callback(sourcemeta::core::JSON::ParsePhase::Post,
                 sourcemeta::core::JSON::Type::Array, end_line, end_column,
                 result);
      }

      return result;
    }

    auto value{consume_value_event(
        parser, &event, callback,
        sourcemeta::core::JSON{static_cast<std::int64_t>(index)})};
    result.push_back(std::move(value));
    index++;
  }
}

auto consume_mapping_events(
    yaml_parser_t *parser, yaml_event_t *start_event,
    const sourcemeta::core::JSON::ParseCallback &callback,
    const sourcemeta::core::JSON &context,
    const yaml_mark_t *override_start_mark = nullptr)
    -> sourcemeta::core::JSON {
  const yaml_mark_t &start_mark{override_start_mark ? *override_start_mark
                                                    : start_event->start_mark};
  const std::uint64_t start_line{start_mark.line + 1};
  const std::uint64_t start_column{start_mark.column + 1};

  if (callback) {
    callback(sourcemeta::core::JSON::ParsePhase::Pre,
             sourcemeta::core::JSON::Type::Object, start_line, start_column,
             context);
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

      if (callback) {
        callback(sourcemeta::core::JSON::ParsePhase::Post,
                 sourcemeta::core::JSON::Type::Object, end_line, end_column,
                 result);
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

    auto value{consume_value_event(parser, &value_event, callback,
                                   sourcemeta::core::JSON{key},
                                   &key_start_mark)};
    result.assign(key, std::move(value));
  }
}

auto consume_value_event(yaml_parser_t *parser, yaml_event_t *event,
                         const sourcemeta::core::JSON::ParseCallback &callback,
                         const sourcemeta::core::JSON &context,
                         const yaml_mark_t *override_start_mark)
    -> sourcemeta::core::JSON {
  sourcemeta::core::JSON result{nullptr};

  switch (event->type) {
    case YAML_SCALAR_EVENT:
      result =
          consume_scalar_event(event, callback, context, override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_SEQUENCE_START_EVENT:
      result = consume_sequence_events(parser, event, callback, context,
                                       override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_MAPPING_START_EVENT:
      result = consume_mapping_events(parser, event, callback, context,
                                      override_start_mark);
      yaml_event_delete(event);
      break;

    case YAML_ALIAS_EVENT:
      yaml_event_delete(event);
      throw sourcemeta::core::YAMLParseError("YAML aliases are not supported");

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

  auto result{consume_value_event(parser, &event, callback,
                                  sourcemeta::core::JSON{nullptr})};

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML document end");
  }
  if (event.type != YAML_DOCUMENT_END_EVENT) {
    yaml_event_delete(&event);
    throw sourcemeta::core::YAMLParseError("Expected YAML document end");
  }
  yaml_event_delete(&event);

  if (!yaml_parser_parse(parser, &event)) {
    throw sourcemeta::core::YAMLParseError("Failed to parse YAML stream end");
  }
  if (event.type != YAML_STREAM_END_EVENT) {
    yaml_event_delete(&event);
    throw sourcemeta::core::YAMLParseError("Expected YAML stream end");
  }
  yaml_event_delete(&event);

  return result;
}

} // namespace

namespace sourcemeta::core {

auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback) -> JSON {
  std::basic_ostringstream<JSON::Char, JSON::CharTraits> buffer;
  buffer << stream.rdbuf();
  return parse_yaml(buffer.str(), callback);
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
  return path.extension() == ".yaml" || path.extension() == ".yml"
             ? read_yaml(path, callback)
             : read_json(path, callback);
}

} // namespace sourcemeta::core
