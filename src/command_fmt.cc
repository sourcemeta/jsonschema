#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <iostream>    // std::cerr, std::cout
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move, std::unreachable

#include "command.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "print.h"
#include "resolver.h"
#include "utils.h"

namespace {

using sourcemeta::jsonschema::format_validation_status;
using sourcemeta::jsonschema::ValidationStatus;

auto diff_operation_name(const sourcemeta::core::Diff::Operation::Type type)
    -> std::string_view {
  switch (type) {
    case sourcemeta::core::Diff::Operation::Type::Equal:
      return "equal";
    case sourcemeta::core::Diff::Operation::Type::Delete:
      return "delete";
    case sourcemeta::core::Diff::Operation::Type::Insert:
      return "insert";
    default:
      std::unreachable();
  }
}

auto to_diff_json(const sourcemeta::core::Diff &difference)
    -> sourcemeta::core::JSON {
  auto operations{sourcemeta::core::JSON::make_array()};

  for (const auto &operation : difference.operations) {
    // An insertion is the only operation that reads the modified input, as
    // the lines it introduces are not present in the original one
    const auto insertion{operation.type ==
                         sourcemeta::core::Diff::Operation::Type::Insert};
    const auto &tokens{insertion ? difference.modified : difference.original};
    const auto start{insertion ? operation.modified_start
                               : operation.original_start};
    const auto end{insertion ? operation.modified_end : operation.original_end};

    auto lines{sourcemeta::core::JSON::make_array()};
    for (auto index{start}; index < end; ++index) {
      lines.push_back(sourcemeta::core::JSON{tokens[index]});
    }

    auto entry{sourcemeta::core::JSON::make_object()};
    entry.assign("type",
                 sourcemeta::core::JSON{diff_operation_name(operation.type)});
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
    std::cerr << format_validation_status(ValidationStatus::Fail) << " "
              << label << "\n";
    sourcemeta::core::Diff::FormatOptions format_options{
        .original_label = "current",
        .modified_label = "expected",
    };
    if (sourcemeta::core::terminal_color_enabled(
            sourcemeta::core::TerminalStream::Stderr)) {
      format_options.line_writer = [](std::ostream &stream,
                                      const sourcemeta::core::Diff::
                                          FormatOptions::LineType type,
                                      const std::string_view prefix,
                                      const std::string_view content) {
        auto style{sourcemeta::core::TerminalStyle::None};
        switch (type) {
          case sourcemeta::core::Diff::FormatOptions::LineType::Delete:
            style = sourcemeta::core::TerminalStyle::Red;
            break;
          case sourcemeta::core::Diff::FormatOptions::LineType::Insert:
            style = sourcemeta::core::TerminalStyle::Green;
            break;
          case sourcemeta::core::Diff::FormatOptions::LineType::Hunk:
            style = sourcemeta::core::TerminalStyle::Cyan;
            break;
          case sourcemeta::core::Diff::FormatOptions::LineType::HeaderOriginal:
          case sourcemeta::core::Diff::FormatOptions::LineType::HeaderModified:
            style = sourcemeta::core::TerminalStyle::Bold;
            break;
          case sourcemeta::core::Diff::FormatOptions::LineType::NoNewline:
            style = sourcemeta::core::TerminalStyle::Yellow;
            break;
          case sourcemeta::core::Diff::FormatOptions::LineType::Context:
            style = sourcemeta::core::TerminalStyle::None;
            break;
        }

        if (style == sourcemeta::core::TerminalStyle::None) {
          stream.write(prefix.data(),
                       static_cast<std::streamsize>(prefix.size()));
          stream.write(content.data(),
                       static_cast<std::streamsize>(content.size()));
        } else {
          std::string line;
          line.reserve(prefix.size() + content.size());
          line.append(prefix);
          line.append(content);
          stream << sourcemeta::jsonschema::paint(
              line, style, sourcemeta::core::TerminalStream::Stderr);
        }
      };
    }

    sourcemeta::core::stringify(difference, std::cerr,
                                sourcemeta::core::Diff::Format::Unified,
                                format_options);
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
    const auto configuration_path{find_configuration(options, current_path)};
    const auto &configuration{
        read_configuration(options, configuration_path, current_path)};
    const auto display_path{stdin_path()};

    std::string raw_stdin;
    const auto parsed{read_from_stdin(&raw_stdin, InputFormatting::Preserve)};
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
          sourcemeta::jsonschema::write_schema(document, expected, indentation,
                                               parsed.roundtrip);
        } else {
          auto copy = document;
          sourcemeta::jsonschema::format_schema(copy, custom_resolver,
                                                effective_dialect);
          sourcemeta::jsonschema::write_schema(copy, expected, indentation,
                                               parsed.roundtrip);
        }

        if (raw_stdin == expected.str()) {
          const auto status =
              output_json ? std::string{"ok:"}
                          : format_validation_status(ValidationStatus::Pass);
          LOG_VERBOSE(options) << status << " " << stdin_label << "\n";
        } else {
          report_check_failure(raw_stdin, expected.str(), stdin_label,
                               output_json, errors);
          result = false;
        }
      } else {
        if (options.contains("keep-ordering")) {
          sourcemeta::jsonschema::write_schema(document, std::cout, indentation,
                                               parsed.roundtrip);
        } else {
          auto copy = document;
          sourcemeta::jsonschema::format_schema(copy, custom_resolver,
                                                effective_dialect);
          sourcemeta::jsonschema::write_schema(copy, std::cout, indentation,
                                               parsed.roundtrip);
        }
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
    if (entry.multidocument) {
      throw MultiDocumentInputError{
          "This command does not support input with multiple documents",
          entry.resolution_base};
    }

    if (!entry.second.is_object() && !entry.second.is_boolean()) {
      throw NotSchemaError{entry.resolution_base};
    }

    if (options.contains("check")) {
      LOG_VERBOSE(options) << "Checking: " << entry.first << "\n";
    } else {
      LOG_VERBOSE(options) << "Formatting: " << entry.first << "\n";
    }

    try {
      const auto configuration_path{
          find_configuration(options, entry.resolution_base)};
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
        sourcemeta::jsonschema::write_schema(entry.second, expected,
                                             indentation, entry.roundtrip);
      } else {
        auto copy = entry.second;
        sourcemeta::jsonschema::format_schema(copy, custom_resolver,
                                              effective_dialect);
        sourcemeta::jsonschema::write_schema(copy, expected, indentation,
                                             entry.roundtrip);
      }

      const auto current{
          sourcemeta::core::read_file_to_string(entry.resolution_base)};

      if (options.contains("check")) {
        if (current == expected.str()) {
          const auto status =
              output_json ? std::string{"ok:"}
                          : format_validation_status(ValidationStatus::Pass);
          LOG_VERBOSE(options) << status << " " << entry.first << "\n";
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
    for (const auto &entry :
         for_each_json(options, InputFormatting::Preserve)) {
      handle_file_entry(entry);
    }
  } else {
    check_no_duplicate_stdin(options.positional());
    for (const auto &arg : options.positional()) {
      if (arg == "-") {
        handle_stdin();
      } else {
        for (const auto &entry :
             for_each_json({arg}, options, InputFormatting::Preserve)) {
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
      constexpr auto HINT_STYLE{sourcemeta::core::TerminalStyle::Bold |
                                sourcemeta::core::TerminalStyle::Cyan};
      std::cerr << "\n"
                << paint("Run the `fmt` command without `--check/-c` to fix "
                         "the formatting",
                         HINT_STYLE, sourcemeta::core::TerminalStream::Stderr)
                << "\n";
    }

    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
