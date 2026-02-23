#include "lexer.h"
#include "parser.h"

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json_error.h>
#include <sourcemeta/core/yaml.h>

#include <sstream> // std::ostringstream

namespace sourcemeta::core {

auto parse_yaml(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
                const JSON::ParseCallback &callback) -> JSON {
  const auto start_pos{stream.tellg()};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  auto result{parser.parse()};

  // Seek stream to position after the parsed document for multi-document
  // support
  const auto consumed{static_cast<std::streamoff>(parser.position())};
  stream.clear();
  stream.seekg(start_pos + consumed);

  return result;
}

auto parse_yaml(const JSON::String &input, const JSON::ParseCallback &callback)
    -> JSON {
  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  return parser.parse();
}

auto read_yaml(const std::filesystem::path &path,
               const JSON::ParseCallback &callback) -> JSON {
  auto stream{read_file(path)};
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  const auto input{buffer.str()};

  yaml::Lexer lexer{input};
  yaml::Parser parser{&lexer, &callback};
  auto result{parser.parse()};

  // After parsing the first document, validate that remaining content
  // is either empty, document markers, or whitespace/comments
  parser.validate_end_of_stream();

  return result;
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
