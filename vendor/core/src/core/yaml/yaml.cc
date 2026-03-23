#include "lexer.h"
#include "parser.h"
#include "stringify.h"

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/yaml.h>

#include <sstream> // std::ostringstream

namespace sourcemeta::core {

auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream)
    -> JSON {
  const auto start_pos{stream.tellg()};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, nullptr};
  auto result{parser.parse()};

  const auto consumed{static_cast<std::streamoff>(parser.position())};
  stream.clear();
  stream.seekg(start_pos + consumed);

  return result;
}

auto parse_yaml(const JSON::String &input) -> JSON {
  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, nullptr};
  return parser.parse();
}

auto read_yaml(const std::filesystem::path &path) -> JSON {
  auto stream{read_file(path)};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, nullptr};
  auto result{parser.parse()};

  parser.validate_end_of_stream();

  return result;
}

auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                JSON &output, const JSON::ParseCallback &callback) -> void {
  const auto start_pos{stream.tellg()};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  output = parser.parse();

  const auto consumed{static_cast<std::streamoff>(parser.position())};
  stream.clear();
  stream.seekg(start_pos + consumed);
}

auto parse_yaml(const JSON::String &input, JSON &output,
                const JSON::ParseCallback &callback) -> void {
  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  output = parser.parse();
}

auto read_yaml(const std::filesystem::path &path, JSON &output,
               const JSON::ParseCallback &callback) -> void {
  auto stream{read_file(path)};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  output = parser.parse();

  parser.validate_end_of_stream();
}

auto read_yaml_or_json(const std::filesystem::path &path) -> JSON {
  const auto extension{path.extension()};
  if (extension == ".yaml" || extension == ".yml") {
    return read_yaml(path);
  } else if (extension == ".json") {
    return read_json(path);
  }

  try {
    return read_json(path);
  } catch (const JSONParseError &) {
    return read_yaml(path);
  }
}

auto read_yaml_or_json(const std::filesystem::path &path, JSON &output,
                       const JSON::ParseCallback &callback) -> void {
  const auto extension{path.extension()};
  if (extension == ".yaml" || extension == ".yml") {
    read_yaml(path, output, callback);
    return;
  } else if (extension == ".json") {
    read_json(path, output, callback);
    return;
  }

  try {
    read_json(path, output, callback);
  } catch (const JSONParseError &) {
    read_yaml(path, output, callback);
  }
}

auto parse_yaml(const JSON::String &input, YAMLRoundTrip &roundtrip) -> JSON {
  roundtrip = {};
  yaml::Lexer lexer{input, true};
  yaml::Parser parser{&lexer, nullptr, &roundtrip};
  return parser.parse();
}

auto parse_yaml(const JSON::String &input, YAMLRoundTrip &roundtrip,
                JSON &output, const JSON::ParseCallback &callback) -> void {
  roundtrip = {};
  yaml::Lexer lexer{input, true};
  yaml::Parser parser{&lexer, &callback, &roundtrip};
  output = parser.parse();
}

auto stringify_yaml(const JSON &document,
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  yaml::stringify_yaml<JSON::Allocator>(document, stream);
}

auto stringify_yaml(const JSON &document,
                    std::basic_ostream<JSON::Char, JSON::CharTraits> &stream,
                    const YAMLRoundTrip &roundtrip) -> void {
  yaml::stringify_yaml<JSON::Allocator>(document, stream, &roundtrip);
}

} // namespace sourcemeta::core
