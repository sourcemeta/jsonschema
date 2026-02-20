#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <algorithm>   // std::any_of
#include <chrono>      // std::chrono
#include <cmath>       // std::sqrt
#include <iostream>    // std::cerr
#include <string>      // std::string
#include <string_view> // std::string_view

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

  // Cannot use stdin for both schema and instances
  if (schema_from_stdin) {
    for (std::size_t i = 1; i < options.positional().size(); ++i) {
      if (options.positional().at(i) == "-") {
        throw StdinError{
            "Cannot read both schema and instance from standard input"};
      }
    }
  }

  if (!schema_from_stdin && std::filesystem::is_directory(schema_path)) {
    throw std::filesystem::filesystem_error{
        "The input was supposed to be a file but it is a directory",
        schema_path, std::make_error_code(std::errc::is_a_directory)};
  }

  // For stdin, use the current working directory as the resolution base
  const auto schema_resolution_base{
      schema_from_stdin ? std::filesystem::current_path()
                        : std::filesystem::path{std::string{schema_path}}};

  const auto configuration_path{find_configuration(schema_resolution_base)};
  const auto &configuration{
      read_configuration(options, configuration_path, schema_resolution_base)};
  const auto dialect{default_dialect(options, configuration)};

  // stdin only supports JSON (cannot retry parsing for YAML)
  const auto schema{schema_from_stdin
                        ? sourcemeta::core::parse_json(std::cin)
                        : sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    throw NotSchemaError{schema_resolution_base};
  }

  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  const auto fast_mode{options.contains("fast")};
  const auto benchmark{options.contains("benchmark")};
  const auto benchmark_loop{parse_loop(options)};
  if (benchmark_loop == 0) {
    throw std::runtime_error("The loop number cannot be zero");
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
      return sourcemeta::core::bundle(schema, sourcemeta::core::schema_walker,
                                      custom_resolver, dialect,
                                      schema_default_id);
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw FileError<sourcemeta::core::SchemaKeywordError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw FileError<sourcemeta::core::SchemaFrameError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      throw FileError<sourcemeta::core::SchemaReferenceError>(
          schema_resolution_base, std::string{error.identifier()},
          error.location(), error.what());
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw FileError<sourcemeta::core::SchemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
          schema_resolution_base);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw FileError<sourcemeta::core::SchemaError>(schema_resolution_base,
                                                     error.what());
    } catch (
        const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
      throw FileError<sourcemeta::core::SchemaReferenceObjectResourceError>(
          schema_resolution_base, error.identifier());
    }
  }()};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};

  try {
    frame.analyse(bundled, sourcemeta::core::schema_walker, custom_resolver,
                  dialect, schema_default_id);
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw FileError<sourcemeta::core::SchemaKeywordError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw FileError<sourcemeta::core::SchemaFrameError>(schema_resolution_base,
                                                        error);
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw FileError<sourcemeta::core::SchemaResolutionError>(
        schema_resolution_base, error);
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
        schema_resolution_base);
  } catch (const sourcemeta::core::SchemaError &error) {
    throw FileError<sourcemeta::core::SchemaError>(schema_resolution_base,
                                                   error.what());
  }

  std::string entrypoint_uri{frame.root()};
  if (options.contains("entrypoint") && !options.at("entrypoint").empty()) {
    try {
      entrypoint_uri = resolve_entrypoint(
          frame, std::string{options.at("entrypoint").front()});
    } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
      throw FileError<sourcemeta::blaze::CompilerInvalidEntryPoint>(
          schema_resolution_base, error);
    }
  }

  const auto schema_template{[&]() {
    try {
      return get_schema_template(bundled, custom_resolver, frame,
                                 entrypoint_uri, fast_mode, options);
    } catch (const sourcemeta::blaze::CompilerInvalidEntryPoint &error) {
      throw FileError<sourcemeta::blaze::CompilerInvalidEntryPoint>(
          schema_resolution_base, error);
    } catch (
        const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
      throw FileError<sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaKeywordError &error) {
      throw FileError<sourcemeta::core::SchemaKeywordError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaFrameError &error) {
      throw FileError<sourcemeta::core::SchemaFrameError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      throw FileError<sourcemeta::core::SchemaReferenceError>(
          schema_resolution_base, std::string{error.identifier()},
          error.location(), error.what());
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      throw FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      throw FileError<sourcemeta::core::SchemaResolutionError>(
          schema_resolution_base, error);
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
          schema_resolution_base);
    } catch (const sourcemeta::core::SchemaError &error) {
      throw FileError<sourcemeta::core::SchemaError>(schema_resolution_base,
                                                     error.what());
    }
  }()};

  sourcemeta::blaze::Evaluator evaluator;

  bool result{true};

  std::vector<std::string_view> instance_arguments;
  if (options.positional().size() > 1) {
    instance_arguments.assign(options.positional().cbegin() + 1,
                              options.positional().cend());
  } else {
    instance_arguments.push_back(".");
  }

  const auto entries = for_each_json(instance_arguments, options);

  if (trace && (entries.size() > 1 ||
                std::any_of(entries.cbegin(), entries.cend(),
                            [](const auto &e) { return e.multidocument; }))) {
    throw std::runtime_error{
        "The `--trace/-t` option is only allowed given a single instance"};
  }

  if (benchmark && entries.size() > 1) {
    // Allow multiple entries from the same JSONL file, but reject
    // multiple source files (directories, multiple arguments)
    const auto &first_path = entries.front().first;
    if (std::any_of(entries.cbegin() + 1, entries.cend(),
                    [&](const auto &e) { return e.first != first_path; })) {
      throw std::runtime_error{
          "The `--benchmark/-b` option is only allowed given a single "
          "instance"};
    }
  }

  for (const auto &entry : entries) {
    std::ostringstream error;
    sourcemeta::blaze::SimpleOutput output{entry.second};
    sourcemeta::blaze::TraceOutput trace_output{
        sourcemeta::core::schema_walker, custom_resolver,
        sourcemeta::core::empty_weak_pointer, frame};
    bool subresult{true};
    if (benchmark) {
      subresult = run_loop(
          evaluator, schema_template, entry.second, entry.first,
          entry.multidocument ? static_cast<std::int64_t>(entry.index + 1)
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
      continue;
    } else if (trace) {
      print(trace_output, entry.positions, std::cout);
      result = subresult;
    } else if (json_output) {
      if (!entry.multidocument && entries.size() > 1) {
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
          break;
        }
      }
    } else if (subresult) {
      LOG_VERBOSE(options)
          << "ok: " << sourcemeta::core::weakly_canonical(entry.first).string();
      if (entry.multidocument) {
        LOG_VERBOSE(options) << " (entry #" << entry.index + 1 << ")";
      }
      LOG_VERBOSE(options)
          << "\n  matches "
          << sourcemeta::core::weakly_canonical(schema_resolution_base).string()
          << "\n";
      print_annotations(output, options, entry.positions, std::cerr);
    } else {
      std::cerr << "fail: "
                << sourcemeta::core::weakly_canonical(entry.first).string();
      if (entry.multidocument) {
        std::cerr << " (entry #" << entry.index + 1 << ")\n\n";
        sourcemeta::core::prettify(entry.second, std::cerr);
        std::cerr << "\n\n";
      } else {
        std::cerr << "\n";
      }
      std::cerr << error.str();
      print(output, entry.positions, std::cerr);
      result = false;
      if (entry.multidocument) {
        break;
      }
    }
  }

  if (!result) {
    throw Fail{2};
  }
}
