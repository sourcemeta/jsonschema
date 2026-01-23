#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>

#include <algorithm> // std::transform
#include <cctype>    // std::toupper
#include <iomanip>   // std::hex, std::setw, std::setfill
#include <iostream>  // std::cerr, std::cout
#include <sstream>   // std::ostringstream

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::compile(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{"This command expects a path to a schema",
                                  "jsonschema compile path/to/schema.json"};
  }

  const auto &schema_path{options.positional().at(0)};
  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_path)};
  const auto dialect{default_dialect(options, configuration)};

  const auto schema{sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_path};
  }

  const auto fast_mode{options.contains("fast")};

  sourcemeta::blaze::Template schema_template;
  try {
    const auto &custom_resolver{
        resolver(options, options.contains("http"), dialect, configuration)};
    schema_template = sourcemeta::blaze::compile(
        schema, sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::blaze::default_schema_compiler,
        fast_mode ? sourcemeta::blaze::Mode::FastValidation
                  : sourcemeta::blaze::Mode::Exhaustive,
        dialect,
        sourcemeta::core::URI::from_path(
            sourcemeta::core::weakly_canonical(schema_path))
            .recompose());
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw FileError<sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw FileError<sourcemeta::core::SchemaKeywordError>(schema_path, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw FileError<sourcemeta::core::SchemaFrameError>(schema_path, error);
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_path, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(schema_path,
                                                             error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_path);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_path, error.what());
  }

  const auto template_json{sourcemeta::blaze::to_json(schema_template)};

  if (options.contains("include") && !options.at("include").empty()) {
    std::string name{options.at("include").front()};

    static const auto IDENTIFIER_PATTERN{
        sourcemeta::core::to_regex("^[A-Za-z_][A-Za-z0-9_]*$")};
    if (!IDENTIFIER_PATTERN.has_value() ||
        !sourcemeta::core::matches(IDENTIFIER_PATTERN.value(), name)) {
      throw InvalidIncludeIdentifier{name};
    }

    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char character) -> unsigned char {
                     return static_cast<unsigned char>(std::toupper(character));
                   });

    std::ostringstream json_stream;
    sourcemeta::core::stringify(template_json, json_stream);
    const auto json_data{std::move(json_stream).str()};

    constexpr auto BYTES_PER_LINE{12};

    std::cout << "#ifndef SOURCEMETA_JSONSCHEMA_INCLUDE_" << name << "_H_\n";
    std::cout << "#define SOURCEMETA_JSONSCHEMA_INCLUDE_" << name << "_H_\n";
    std::cout << "\n";
    std::cout << "#ifdef __cplusplus\n";
    std::cout << "#include <cstddef>\n";
    std::cout << "#include <string_view>\n";
    std::cout << "#endif\n";
    std::cout << "\n";
    std::cout << "static const char " << name << "_DATA[] = {";

    for (std::size_t index = 0; index < json_data.size(); ++index) {
      if (index % BYTES_PER_LINE == 0) {
        std::cout << "\n  ";
      }

      std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0')
                << (static_cast<unsigned int>(
                       static_cast<unsigned char>(json_data[index])));

      std::cout << ",";
      if ((index + 1) % BYTES_PER_LINE != 0) {
        std::cout << " ";
      }
    }

    if (json_data.size() % BYTES_PER_LINE != 0) {
      std::cout << "0x00";
    } else {
      std::cout << "\n  0x00";
    }

    std::cout << "\n};\n";
    std::cout << "\n";
    std::cout << std::dec;
    std::cout << "static const unsigned int " << name
              << "_LENGTH = " << json_data.size() << ";\n";
    std::cout << "\n";
    std::cout << "#ifdef __cplusplus\n";
    std::cout << "static constexpr std::string_view " << name << "{" << name
              << "_DATA, " << name << "_LENGTH};\n";
    std::cout << "#endif\n";
    std::cout << "\n";
    std::cout << "#endif\n";
  } else if (options.contains("minify")) {
    sourcemeta::core::stringify(template_json, std::cout);
    std::cout << "\n";
  } else {
    sourcemeta::core::prettify(template_json, std::cout);
    std::cout << "\n";
  }
}
