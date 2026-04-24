#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <fstream>  // std::ofstream
#include <iostream> // std::cerr, std::cout
#include <sstream>  // std::ostringstream
#include <utility>  // std::move
#include <vector>   // std::vector

#include "command.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

auto sourcemeta::jsonschema::fmt(const sourcemeta::core::Options &options)
    -> void {
  const bool output_json{options.contains("json")};
  bool result{true};
  std::vector<std::string> failed_files;
  const auto indentation{parse_indentation(options)};

  const auto handle_stdin = [&]() {
    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    const auto &configuration{
        read_configuration(options, configuration_path, current_path)};
    const auto display_path{stdin_path()};

    std::string raw_stdin;
    const auto parsed{read_from_stdin(&raw_stdin)};
    if (parsed.yaml) {
      throw YAMLInputError{"This command does not support YAML input files yet",
                           display_path};
    }

    const auto &document{parsed.document};
    const auto dialect{default_dialect(options, configuration)};
    const auto is_test_document =
        dialect.empty() && looks_like_test_document(document);
    const auto effective_dialect =
        is_test_document ? TEST_DOCUMENT_DEFAULT_DIALECT : dialect;
    if (is_test_document) {
      std::cerr << "Interpreting as a test file: " << display_path.string()
                << "\n";
    }
    const auto &custom_resolver{resolver(options, options.contains("http"),
                                         effective_dialect, configuration)};
    const auto stdin_label{display_path.string()};

    try {
      if (options.contains("check")) {
        std::ostringstream expected;
        if (options.contains("keep-ordering")) {
          sourcemeta::core::prettify(document, expected, indentation);
        } else {
          auto copy = document;
          sourcemeta::core::format(copy, sourcemeta::core::schema_walker,
                                   custom_resolver, effective_dialect);
          sourcemeta::core::prettify(copy, expected, indentation);
        }
        expected << "\n";

        if (raw_stdin == expected.str()) {
          LOG_VERBOSE(options) << "ok: " << stdin_label << "\n";
        } else if (output_json) {
          failed_files.push_back(stdin_label);
          result = false;
        } else {
          std::cerr << "fail: " << stdin_label << "\n";
          result = false;
        }
      } else {
        if (options.contains("keep-ordering")) {
          sourcemeta::core::prettify(document, std::cout, indentation);
        } else {
          auto copy = document;
          sourcemeta::core::format(copy, sourcemeta::core::schema_walker,
                                   custom_resolver, effective_dialect);
          sourcemeta::core::prettify(copy, std::cout, indentation);
        }
        std::cout << "\n";
      }
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          display_path, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          display_path, error);
    } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
      const auto position{parsed.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            display_path, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>(display_path, error);
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          display_path, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(display_path, error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(display_path);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          display_path, error.what());
    }
  };

  const auto handle_file_entry = [&](const InputJSON &entry) {
    if (entry.yaml) {
      throw YAMLInputError{"This command does not support YAML input files yet",
                           entry.resolution_base};
    }

    if (options.contains("check")) {
      LOG_VERBOSE(options) << "Checking: " << entry.first << "\n";
    } else {
      LOG_VERBOSE(options) << "Formatting: " << entry.first << "\n";
    }

    try {
      const auto configuration_path{find_configuration(entry.resolution_base)};
      const auto &configuration{read_configuration(options, configuration_path,
                                                   entry.resolution_base)};
      const auto dialect{default_dialect(options, configuration)};
      const auto is_test_document =
          dialect.empty() && looks_like_test_document(entry.second);
      const auto effective_dialect =
          is_test_document ? TEST_DOCUMENT_DEFAULT_DIALECT : dialect;
      if (is_test_document) {
        std::cerr << "Interpreting as a test file: " << entry.first << "\n";
      }
      const auto &custom_resolver{resolver(options, options.contains("http"),
                                           effective_dialect, configuration)};

      std::ostringstream expected;
      if (options.contains("keep-ordering")) {
        sourcemeta::core::prettify(entry.second, expected, indentation);
      } else {
        auto copy = entry.second;
        sourcemeta::core::format(copy, sourcemeta::core::schema_walker,
                                 custom_resolver, effective_dialect);
        sourcemeta::core::prettify(copy, expected, indentation);
      }
      expected << "\n";

      std::ifstream current_stream{entry.resolution_base};
      std::ostringstream current;
      current << current_stream.rdbuf();

      if (options.contains("check")) {
        if (current.str() == expected.str()) {
          LOG_VERBOSE(options) << "ok: " << entry.first << "\n";
        } else if (output_json) {
          failed_files.push_back(entry.first);
          result = false;
        } else {
          std::cerr << "fail: " << entry.first << "\n";
          result = false;
        }
      } else {
        if (current.str() != expected.str()) {
          std::ofstream output{entry.resolution_base};
          output << expected.str();
        }
      }
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::core::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaAnchorCollisionError>(entry.resolution_base,
                                                        error);
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(entry.resolution_base,
                                                   error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(
          entry.resolution_base);
    } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          entry.resolution_base, error.what());
    }
  };

  // Process arguments in order to preserve argument ordering semantics.
  // When no positional arguments are given, default to for_each_json(options)
  // which scans the current directory.
  if (options.positional().empty()) {
    for (const auto &entry : for_each_json(options)) {
      handle_file_entry(entry);
    }
  } else {
    check_no_duplicate_stdin(options.positional());
    for (const auto &arg : options.positional()) {
      if (arg == "-") {
        handle_stdin();
      } else {
        for (const auto &entry : for_each_json({arg}, options)) {
          handle_file_entry(entry);
        }
      }
    }
  }

  if (options.contains("check") && output_json) {
    auto output_json_object{sourcemeta::core::JSON::make_object()};
    output_json_object.assign("valid", sourcemeta::core::JSON{result});

    if (!result) {
      auto errors_array{sourcemeta::core::JSON::make_array()};
      for (auto &file_path : failed_files) {
        errors_array.push_back(sourcemeta::core::JSON{std::move(file_path)});
      }

      output_json_object.assign("errors", sourcemeta::core::JSON{errors_array});
    }

    sourcemeta::core::prettify(output_json_object, std::cout, indentation);
    std::cout << "\n";
  }

  if (!result) {
    if (!output_json) {
      std::cerr << "\nRun the `fmt` command without `--check/-c` to fix the "
                   "formatting"
                << "\n";
    }

    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
