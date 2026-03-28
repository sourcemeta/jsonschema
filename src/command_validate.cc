#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <chrono>      // std::chrono
#include <cmath>       // std::sqrt
#include <iostream>    // std::cerr
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::as_const

#include "command.h"
#include "configuration.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "resolver.h"
#include "utils.h"

namespace {

auto get_precompiled_schema_template_path(
    const sourcemeta::core::Options &options)
    -> std::optional<std::filesystem::path> {
  if (options.contains("template") && !options.at("template").empty()) {
    return options.at("template").front();
  } else {
    return std::nullopt;
  }
}

auto get_schema_template(const sourcemeta::core::JSON &bundled,
                         const sourcemeta::core::SchemaResolver &resolver,
                         const sourcemeta::core::SchemaFrame &frame,
                         const std::string &entrypoint_uri,
                         const bool fast_mode,
                         const sourcemeta::core::Options &options)
    -> sourcemeta::blaze::Template {
  const auto precompiled{get_precompiled_schema_template_path(options)};
  if (precompiled.has_value()) {
    sourcemeta::jsonschema::LOG_VERBOSE(options)
        << "Parsing pre-compiled schema template: "
        << sourcemeta::core::weakly_canonical(precompiled.value()).string()
        << "\n";
    const auto schema_template{
        sourcemeta::core::read_yaml_or_json(precompiled.value())};
    const auto precompiled_result{
        sourcemeta::blaze::from_json(schema_template)};
    if (precompiled_result.has_value()) {
      return precompiled_result.value();
    } else {
      sourcemeta::jsonschema::LOG_WARNING()
          << "Failed to parse pre-compiled schema template. "
             "Compiling from scratch\n";
    }
  }

  return sourcemeta::blaze::compile(
      bundled, sourcemeta::core::schema_walker, resolver,
      sourcemeta::blaze::default_schema_compiler, frame, entrypoint_uri,
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive);
}

auto parse_loop(const sourcemeta::core::Options &options) -> std::uint64_t {
  if (options.contains("loop")) {
    return std::stoull(options.at("loop").front().data());
  } else {
    return 1;
  }
}

// validate instance in a loop to measure avg and stdev
auto run_loop(sourcemeta::blaze::Evaluator &evaluator,
              const sourcemeta::blaze::Template &schema_template,
              const sourcemeta::core::JSON &instance,
              const std::string &instance_path, const int64_t instance_index,
              const uint64_t loop) -> bool {
  const auto iterations = static_cast<double>(loop);
  double sum = 0.0, sum2 = 0.0, empty = 0.0;
  bool result = true;

  // Overhead evaluation, if not to optimize out!
  for (auto index = loop; index; index--) {
    const auto start{std::chrono::high_resolution_clock::now()};
    const auto end{std::chrono::high_resolution_clock::now()};
    empty +=
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count()) /
        1000.0;
  }
  empty /= iterations;

  // Actual performance loop
  for (auto index = loop; index; index--) {
    const auto start{std::chrono::high_resolution_clock::now()};
    result = evaluator.validate(schema_template, instance);
    const auto end{std::chrono::high_resolution_clock::now()};

    const auto raw_delay =
        static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                .count()) /
        1000.0;
    const auto delay = std::max(0.0, raw_delay - empty);
    sum += delay;
    sum2 += delay * delay;
  }

  // Display json source, result and performance
  auto avg = sum / iterations;
  auto stdev = loop == 1 ? 0.0 : std::sqrt(sum2 / iterations - avg * avg);

  std::cout << instance_path;
  if (instance_index >= 0)
    std::cout << "[" << instance_index << "]";
  std::cout << std::fixed;
  std::cout.precision(3);
  std::cout << ": " << (result ? "PASS" : "FAIL") << " " << avg << " +- "
            << stdev << " us (" << empty << ")\n";

  return result;
}

// Returns false if iteration should stop
auto process_entry(
    const sourcemeta::jsonschema::InputJSON &entry,
    sourcemeta::blaze::Evaluator &evaluator,
    const sourcemeta::blaze::Template &schema_template,
    const sourcemeta::jsonschema::CustomResolver &custom_resolver,
    const sourcemeta::core::SchemaFrame &frame, bool benchmark,
    std::uint64_t benchmark_loop, bool trace, bool fast_mode, bool json_output,
    bool schema_from_stdin, const std::filesystem::path &schema_resolution_base,
    const sourcemeta::core::Options &options, bool &result) -> bool {
  std::ostringstream error;
  sourcemeta::blaze::SimpleOutput output{entry.second};
  sourcemeta::blaze::TraceOutput trace_output{
      sourcemeta::core::schema_walker, custom_resolver,
      sourcemeta::jsonschema::trace_callback(entry.positions, std::cout),
      sourcemeta::core::empty_weak_pointer, frame};
  bool subresult{true};
  if (benchmark) {
    subresult = run_loop(evaluator, schema_template, entry.second, entry.first,
                         entry.multidocument
                             ? static_cast<std::int64_t>(entry.index + 1)
                             : static_cast<std::int64_t>(-1),
                         benchmark_loop);
    if (!subresult) {
      error << "error: Schema validation failure\n";
      result = false;
    }
  } else if (trace) {
    subresult = evaluator.validate(schema_template, entry.second,
                                   std::ref(trace_output));
  } else if (fast_mode) {
    subresult = evaluator.validate(schema_template, entry.second);
  } else if (!json_output) {
    subresult =
        evaluator.validate(schema_template, entry.second, std::ref(output));
  }

  if (benchmark) {
    return true;
  } else if (trace) {
    result = result && subresult;
  } else if (json_output) {
    if (!entry.multidocument) {
      std::cerr << entry.first << "\n";
    }
    const auto suboutput{sourcemeta::blaze::standard(
        evaluator, schema_template, entry.second,
        fast_mode ? sourcemeta::blaze::StandardOutput::Flag
                  : sourcemeta::blaze::StandardOutput::Basic,
        entry.positions)};
    assert(suboutput.is_object());
    assert(suboutput.defines("valid"));
    assert(suboutput.at("valid").is_boolean());
    sourcemeta::core::prettify(suboutput, std::cout);
    std::cout << "\n";
    if (!suboutput.at("valid").to_boolean()) {
      result = false;
      if (entry.multidocument) {
        return false;
      }
    }
  } else if (subresult) {
    sourcemeta::jsonschema::LOG_VERBOSE(options) << "ok: " << entry.first;
    if (entry.multidocument) {
      sourcemeta::jsonschema::LOG_VERBOSE(options)
          << " (entry #" << entry.index + 1 << ")";
    }
    sourcemeta::jsonschema::LOG_VERBOSE(options)
        << "\n  matches "
        << (schema_from_stdin
                ? "/dev/stdin"
                : sourcemeta::core::weakly_canonical(schema_resolution_base)
                      .string())
        << "\n";
    sourcemeta::jsonschema::print_annotations(output, options, entry.positions,
                                              std::cerr);
  } else {
    std::cerr << "fail: " << entry.first;
    if (entry.multidocument) {
      std::cerr << " (entry #" << entry.index + 1 << ")\n\n";
      sourcemeta::core::prettify(entry.second, std::cerr);
      std::cerr << "\n\n";
    } else {
      std::cerr << "\n";
    }
    std::cerr << error.str();
    sourcemeta::jsonschema::print(output, entry.positions, std::cerr);
    result = false;
    if (entry.multidocument) {
      return false;
    }
  }

  return true;
}

} // namespace

auto sourcemeta::jsonschema::validate(const sourcemeta::core::Options &options)
    -> void {
  if (options.positional().size() < 1) {
    throw PositionalArgumentError{
        "This command expects a path to a schema and a path to an\n"
        "instance to validate against the schema",
        "jsonschema validate path/to/schema.json path/to/instance.json"};
  }

  const auto &schema_path{options.positional().at(0)};
  const bool schema_from_stdin = (schema_path == "-");

  // Centralized duplicate stdin check for all positional arguments
  check_no_duplicate_stdin(options.positional());

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory",
        schema_path, std::make_error_code(std::errc::is_a_directory)};
  }

  if (options.contains("path") && !options.at("path").empty() &&
      options.contains("template") && !options.at("template").empty()) {
    throw OptionConflictError{
        "The --path option cannot be used with --template"};
  }

  const auto schema_config_base{schema_from_stdin
                                    ? std::filesystem::current_path()
                                    : std::filesystem::path(schema_path)};
  const auto schema_resolution_base{
      schema_from_stdin ? stdin_path() : std::filesystem::path(schema_path)};

  const auto configuration_path{find_configuration(schema_config_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_config_base)};
  const auto dialect{default_dialect(options, configuration)};

  sourcemeta::core::JSON schema{
      schema_from_stdin ? read_from_stdin().document
                        : sourcemeta::core::read_yaml_or_json(schema_path)};

  if (options.contains("path") && !options.at("path").empty()) {
    // Invalid pointer syntax is handled by to_pointer(), consistent with
    // --entrypoint behavior.
    const auto path_string{std::string{options.at("path").front()}};
    const auto pointer{sourcemeta::core::to_pointer(path_string)};
    const auto *const result{sourcemeta::core::try_get(schema, pointer)};
    // We intentionally reuse NotSchemaError here to align with existing CLI
    // error semantics without introducing a new error type.
    if (!result) {
      throw NotSchemaError{schema_resolution_base};
    }
    // Note: extracting a sub-schema may break $ref references outside the
    // selected subtree. This is expected behavior for --path given the current
    // CLI design.
    // `result` points into `schema`, so we must copy before reassigning to
    // avoid a use-after-free (the copy assignment destroys schema's storage
    // before reading from other when they alias).
    sourcemeta::core::JSON subschema{*result};
    schema = std::move(subschema);
  }

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_from_stdin ? stdin_path()
                                           : schema_resolution_base};
  }

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  const auto fast_mode{options.contains("fast")};
  const auto benchmark{options.contains("benchmark")};
  const auto benchmark_loop{parse_loop(options)};
  if (benchmark_loop == 0) {
    throw OptionConflictError{"The loop number cannot be zero"};
  }

  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  if (options.contains("entrypoint") && !options.at("entrypoint").empty() &&
      options.contains("template") && !options.at("template").empty()) {
    throw OptionConflictError{
        "The --entrypoint option cannot be used with --template"};
  }

  const auto schema_default_id{
      sourcemeta::jsonschema::default_id(schema_resolution_base)};

  const sourcemeta::core::JSON bundled{[&]() {
    try {
      return sourcemeta::core::bundle(
          std::as_const(schema), sourcemeta::core::schema_walker,
          custom_resolver, dialect, schema_default_id);
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
          schema_resolution_base, std::string{error.identifier()},
          error.location(), error.what());
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(schema_resolution_base,
                                                   error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(
          schema_resolution_base);
    } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownDialectError>(schema_resolution_base);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          schema_resolution_base, error.what());
    } catch (
        const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaReferenceObjectResourceError>(
          schema_resolution_base, error.identifier());
    }
  }()};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};

  try {
    frame.analyse(bundled, sourcemeta::core::schema_walker, custom_resolver,
                  dialect, schema_default_id);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
        schema_resolution_base, error);
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>(schema_resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
        schema_resolution_base, error.what());
  }

  std::string entrypoint_uri{frame.root()};
  if (options.contains("entrypoint") && !options.at("entrypoint").empty()) {
    try {
      entrypoint_uri = resolve_entrypoint(
          frame, std::string{options.at("entrypoint").front()});
    } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidEntryPoint>(schema_resolution_base,
                                                        error);
    }
  }

  const auto schema_template{[&]() {
    try {
      return get_schema_template(bundled, custom_resolver, frame,
                                 entrypoint_uri, fast_mode, options);
    } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidEntryPoint>(schema_resolution_base,
                                                        error);
    } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerInvalidRegexError>(schema_resolution_base,
                                                        error);
    } catch (
        const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>(
          schema_resolution_base, std::string{error.identifier()},
          error.location(), error.what());
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaResolutionError>(schema_resolution_base,
                                                   error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownBaseDialectError>(
          schema_resolution_base);
    } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaUnknownDialectError>(schema_resolution_base);
    } catch (const sourcemeta::core::SchemaVocabularyError &error) {
      throw sourcemeta::core::FileError<
          sourcemeta::core::SchemaVocabularyError>(
          schema_resolution_base, std::string{error.uri()}, error.what());
    } catch (const sourcemeta::core::SchemaError &error) {
      throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>(
          schema_resolution_base, error.what());
    }
  }()};

  sourcemeta::blaze::Evaluator evaluator;

  bool result{true};

  std::vector<std::string_view> instance_arguments;
  if (options.positional().size() > 1) {
    instance_arguments.assign(options.positional().cbegin() + 1,
                              options.positional().cend());
  }

  if (trace && benchmark) {
    throw OptionConflictError{
        "The `--trace/-t` and `--benchmark/-b` options are mutually exclusive"};
  }

  if (trace && instance_arguments.size() > 1) {
    throw OptionConflictError{
        "The `--trace/-t` option is only allowed given a single instance"};
  }

  if (benchmark && instance_arguments.size() > 1) {
    throw OptionConflictError{
        "The `--benchmark/-b` option is only allowed given a single instance"};
  }

  if (instance_arguments.empty()) {
    if (trace) {
      throw OptionConflictError{
          "The `--trace/-t` option is only allowed given a single instance"};
    }

    if (benchmark) {
      throw OptionConflictError{"The `--benchmark/-b` option is only allowed "
                                "given a single instance"};
    }

    for (const auto &entry : for_each_json({}, options)) {
      if (!process_entry(entry, evaluator, schema_template, custom_resolver,
                         frame, benchmark, benchmark_loop, trace, fast_mode,
                         json_output, schema_from_stdin, schema_resolution_base,
                         options, result)) {
        break;
      }
    }
  } else {
    for (const auto &instance_path_view : instance_arguments) {
      const std::filesystem::path instance_path{instance_path_view};
      if (trace && instance_path.extension() == ".jsonl") {
        throw OptionConflictError{
            "The `--trace/-t` option is only allowed given a single instance"};
      }

      if (trace && instance_path_view != "-" &&
          std::filesystem::is_directory(instance_path)) {
        throw OptionConflictError{
            "The `--trace/-t` option is only allowed given a single instance"};
      }

      if (benchmark && instance_path_view != "-" &&
          std::filesystem::is_directory(instance_path)) {
        throw OptionConflictError{"The `--benchmark/-b` option is only allowed "
                                  "given a single instance"};
      }

      if (instance_path_view == "-" ||
          std::filesystem::is_directory(instance_path) ||
          instance_path.extension() == ".jsonl" ||
          instance_path.extension() == ".yaml" ||
          instance_path.extension() == ".yml") {
        for (const auto &entry : for_each_json({instance_path_view}, options)) {
          if (!process_entry(entry, evaluator, schema_template, custom_resolver,
                             frame, benchmark, benchmark_loop, trace, fast_mode,
                             json_output, schema_from_stdin,
                             schema_resolution_base, options, result)) {
            break;
          }
        }
      } else {
        sourcemeta::core::PointerPositionTracker tracker;
        auto property_storage = std::make_shared<std::deque<std::string>>();
        const bool track_positions{(!fast_mode && !benchmark) || trace};
        const auto instance{[&]() -> sourcemeta::core::JSON {
          if (track_positions) {
            sourcemeta::core::JSON document{sourcemeta::core::JSON{nullptr}};
            auto callback = make_position_callback(tracker, property_storage);
            sourcemeta::core::read_yaml_or_json(instance_path, document,
                                                callback);
            return document;
          }
          return sourcemeta::core::read_yaml_or_json(instance_path);
        }()};
        std::ostringstream error;
        sourcemeta::blaze::SimpleOutput output{instance};
        sourcemeta::blaze::TraceOutput trace_output{
            sourcemeta::core::schema_walker, custom_resolver,
            trace_callback(tracker, std::cout),
            sourcemeta::core::empty_weak_pointer, frame};
        bool subresult{true};
        if (benchmark) {
          subresult =
              run_loop(evaluator, schema_template, instance,
                       instance_path.string(), (int64_t)-1, benchmark_loop);
          if (!subresult) {
            error << "error: Schema validation failure\n";
            result = false;
          }
        } else if (trace) {
          subresult = evaluator.validate(schema_template, instance,
                                         std::ref(trace_output));
        } else if (fast_mode) {
          subresult = evaluator.validate(schema_template, instance);
        } else if (!json_output) {
          subresult =
              evaluator.validate(schema_template, instance, std::ref(output));
        }

        if (trace) {
          result = result && subresult;
        } else if (json_output) {
          const auto suboutput{sourcemeta::blaze::standard(
              evaluator, schema_template, instance,
              fast_mode ? sourcemeta::blaze::StandardOutput::Flag
                        : sourcemeta::blaze::StandardOutput::Basic,
              tracker)};
          assert(suboutput.is_object());
          assert(suboutput.defines("valid"));
          assert(suboutput.at("valid").is_boolean());
          if (!suboutput.at("valid").to_boolean()) {
            result = false;
          }

          sourcemeta::core::prettify(suboutput, std::cout);
          std::cout << "\n";
        } else if (subresult) {
          LOG_VERBOSE(options)
              << "ok: "
              << sourcemeta::core::weakly_canonical(instance_path).string()
              << "\n  matches "
              << (schema_from_stdin ? "/dev/stdin"
                                    : sourcemeta::core::weakly_canonical(
                                          schema_resolution_base)
                                          .string())
              << "\n";
          print_annotations(output, options, tracker, std::cerr);
        } else {
          std::cerr
              << "fail: "
              << sourcemeta::core::weakly_canonical(instance_path).string()
              << "\n";
          std::cerr << error.str();
          print(output, tracker, std::cerr);
          result = false;
        }
      }
    }
  }

  if (!result) {
    throw Fail{EXIT_EXPECTED_FAILURE};
  }
}
