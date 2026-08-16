#include <sourcemeta/blaze/format.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>

#include <iostream> // std::cerr, std::cout
#include <sstream>  // std::ostringstream
#include <string>   // std::string
#include <utility>  // std::move

#include "command.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto to_diff_json(const sourcemeta::core::Diff &difference)
    -> sourcemeta::core::JSON {
  auto operations{sourcemeta::core::JSON::make_array()};

  for (const auto &operation : difference.operations) {
    auto entry{sourcemeta::core::JSON::make_object()};
    auto lines{sourcemeta::core::JSON::make_array()};

    switch (operation.type) {
      case sourcemeta::core::Diff::Operation::Type::Equal:
        entry.assign("type", sourcemeta::core::JSON{"equal"});
        for (auto index{operation.original_start};
             index < operation.original_end; ++index) {
          lines.push_back(sourcemeta::core::JSON{difference.original[index]});
        }

        break;
      case sourcemeta::core::Diff::Operation::Type::Delete:
        entry.assign("type", sourcemeta::core::JSON{"delete"});
        for (auto index{operation.original_start};
             index < operation.original_end; ++index) {
          lines.push_back(sourcemeta::core::JSON{difference.original[index]});
        }

        break;
      case sourcemeta::core::Diff::Operation::Type::Insert:
        entry.assign("type", sourcemeta::core::JSON{"insert"});
        for (auto index{operation.modified_start};
             index < operation.modified_end; ++index) {
          lines.push_back(sourcemeta::core::JSON{difference.modified[index]});
        }

        break;
    }

    entry.assign("lines", std::move(lines));
    operations.push_back(std::move(entry));
  }

  return operations;
}

auto report_check_failure(const std::string &current,
                          const std::string &expected, const std::string &label,
                          const bool output_json,
                          sourcemeta::core::JSON &errors) -> void {
  const auto difference{sourcemeta::core::diff(
      current, expected, sourcemeta::core::Diff::Mode::Line,
      sourcemeta::core::Diff::Algorithm::Myers)};

  if (output_json) {
    auto entry{sourcemeta::core::JSON::make_object()};
    entry.assign("path", sourcemeta::core::JSON{label});
    entry.assign("diff", to_diff_json(difference));
    errors.push_back(std::move(entry));
  } else {
    std::cerr << "fail: " << label << "\n";
    sourcemeta::core::stringify(
        difference, std::cerr, sourcemeta::core::Diff::Format::Unified,
        {.original_label = "current", .modified_label = "expected"});
  }
}

} // namespace

auto sourcemeta::jsonschema::fmt(const sourcemeta::core::Options &options)
    -> void {
  validate_http_headers(options);
  const bool output_json{options.contains("json")};
  bool result{true};
  auto errors{sourcemeta::core::JSON::make_array()};
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
      std::cerr << "Interpreting as a test file: "
                << display_path.generic_string() << "\n";
    }
    const auto &custom_resolver{resolver(options, options.contains("http"),
                                         effective_dialect, configuration)};
    const auto stdin_label{display_path.generic_string()};

    try {
      if (options.contains("check")) {
        std::ostringstream expected;
        if (options.contains("keep-ordering")) {
          sourcemeta::core::prettify(document, expected, indentation);
        } else {
          auto copy = document;
          sourcemeta::blaze::format(copy, sourcemeta::blaze::schema_walker,
                                    custom_resolver, effective_dialect);
          sourcemeta::core::prettify(copy, expected, indentation);
        }
        expected << "\n";

        if (raw_stdin == expected.str()) {
          LOG_VERBOSE(options) << "ok: " << stdin_label << "\n";
        } else {
          report_check_failure(raw_stdin, expected.str(), stdin_label,
                               output_json, errors);
          result = false;
        }
      } else {
        if (options.contains("keep-ordering")) {
          sourcemeta::core::prettify(document, std::cout, indentation);
        } else {
          auto copy = document;
          sourcemeta::blaze::format(copy, sourcemeta::blaze::schema_walker,
                                    custom_resolver, effective_dialect);
          sourcemeta::core::prettify(copy, std::cout, indentation);
        }
        std::cout << "\n";
      }
    } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
          display_path, error);
    } catch (const sourcemeta::blaze::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
          display_path, error);
    } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
      const auto position{parsed.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::blaze::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            display_path, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>(display_path, error);
    } catch (const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
          display_path, error);
    } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaResolutionError>(display_path, error);
    } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaUnknownBaseDialectError>(display_path);
    } catch (const sourcemeta::blaze::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
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
        sourcemeta::blaze::format(copy, sourcemeta::blaze::schema_walker,
                                  custom_resolver, effective_dialect);
        sourcemeta::core::prettify(copy, expected, indentation);
      }
      expected << "\n";

      const auto current{
          sourcemeta::core::read_file_to_string(entry.resolution_base)};

      if (options.contains("check")) {
        if (current == expected.str()) {
          LOG_VERBOSE(options) << "ok: " << entry.first << "\n";
        } else {
          report_check_failure(current, expected.str(), entry.first,
                               output_json, errors);
          result = false;
        }
      } else {
        if (current != expected.str()) {
          sourcemeta::core::atomic_write_file(entry.resolution_base,
                                              expected.str());
        }
      }
    } catch (const sourcemeta::blaze::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaAnchorCollisionError &error) {
      const auto position{entry.positions.get(error.location())};
      if (position.has_value()) {
        throw PositionError<sourcemeta::core::FileError<
            sourcemeta::blaze::SchemaAnchorCollisionError>>(
            std::get<0>(position.value()), std::get<1>(position.value()),
            entry.resolution_base, error);
      }

      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaAnchorCollisionError>(entry.resolution_base,
                                                         error);
    } catch (const sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError>(
          entry.resolution_base, error);
    } catch (const sourcemeta::blaze::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaResolutionError>(entry.resolution_base,
                                                    error);
    } catch (const sourcemeta::blaze::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaUnknownBaseDialectError>(
          entry.resolution_base);
    } catch (const sourcemeta::blaze::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::SchemaUnknownDialectError>(entry.resolution_base);
    } catch (const sourcemeta::blaze::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>(
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
      output_json_object.assign("errors", std::move(errors));
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
