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
                         const std::string_view default_dialect,
                         const std::string_view default_id,
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
      sourcemeta::blaze::default_schema_compiler, frame,
      fast_mode ? sourcemeta::blaze::Mode::FastValidation
                : sourcemeta::blaze::Mode::Exhaustive,
      default_dialect, default_id);
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
              const std::filesystem::path &instance_path,
              const int64_t instance_index, const uint64_t loop) -> bool {
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

  std::cout << instance_path.string();
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

  const auto &schema_input{options.positional().at(0)};
  const bool schema_is_url{sourcemeta::jsonschema::is_http_url(schema_input)};

  std::optional<std::filesystem::path> schema_path{std::nullopt};
  const std::optional<sourcemeta::core::SchemaConfig> *configuration{nullptr};
  const sourcemeta::jsonschema::CustomResolver *custom_resolver{nullptr};
  std::string_view dialect;
  sourcemeta::core::JSON schema{sourcemeta::core::JSON::make_object()};
  std::string schema_display;
  std::string default_id;

  if (schema_is_url) {
    if (!options.contains("http")) {
      throw std::runtime_error{
          "Remote schema inputs require network access. Pass `--http/-h`"};
    }

    const auto current_path{std::filesystem::current_path()};
    const auto configuration_path{find_configuration(current_path)};
    configuration = &read_configuration(options, configuration_path);
    dialect = default_dialect(options, *configuration);
    custom_resolver =
        &resolver(options, /* remote */ true, dialect, *configuration);

    schema = sourcemeta::jsonschema::fetch_http_schema(options, schema_input);
    schema_display = std::string{schema_input};
    default_id = schema_display;
  } else {
    schema_path = std::filesystem::path{schema_input};

    if (std::filesystem::is_directory(schema_path.value())) {
      throw std::filesystem::filesystem_error{
          "The input was supposed to be a file but it is a directory",
          schema_path.value(), std::make_error_code(std::errc::is_a_directory)};
    }

    const auto configuration_path{find_configuration(schema_path.value())};
    configuration =
        &read_configuration(options, configuration_path, schema_path.value());
    dialect = default_dialect(options, *configuration);
    schema = sourcemeta::core::read_yaml_or_json(schema_path.value());
    schema_display =
        sourcemeta::core::weakly_canonical(schema_path.value()).string();
    default_id = sourcemeta::core::URI::from_path(
                     sourcemeta::core::weakly_canonical(schema_path.value()))
                     .recompose();
    custom_resolver =
        &resolver(options, options.contains("http"), dialect, *configuration);
  }

  if (!sourcemeta::core::is_schema(schema)) {
    if (schema_path.has_value()) {
      throw NotSchemaError{schema_path.value()};
    }

    throw RemoteSchemaNotSchemaError{std::string{schema_input}};
  }

  assert(custom_resolver != nullptr);

  const auto fast_mode{options.contains("fast")};
  const auto benchmark{options.contains("benchmark")};
  const auto benchmark_loop{parse_loop(options)};
  if (benchmark_loop == 0) {
    throw std::runtime_error("The loop number cannot be zero");
  }

  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  const sourcemeta::core::JSON bundled{[&]() {
    try {
      return sourcemeta::core::bundle(
          static_cast<const sourcemeta::core::JSON &>(schema),
          sourcemeta::core::schema_walker, *custom_resolver, dialect,
          default_id);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaReferenceError>(
            schema_path.value(), std::string{error.identifier()},
            error.location(), error.what());
      }

      throw;
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      if (schema_path.has_value()) {
        throw FileError<
            sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
            schema_path.value());
      }

      throw;
    } catch (const sourcemeta::core::SchemaError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaError>(schema_path.value(),
                                                       error.what());
      }

      throw;
    } catch (
        const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaReferenceObjectResourceError>(
            schema_path.value(), error.identifier());
      }

      throw;
    }
  }()};

  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};

  try {
    frame.analyse(bundled, sourcemeta::core::schema_walker, *custom_resolver,
                  dialect, default_id);
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    if (schema_path.has_value()) {
      throw FileError<
          sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
          schema_path.value(), error);
    }

    throw;
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    if (schema_path.has_value()) {
      throw FileError<sourcemeta::core::SchemaResolutionError>(
          schema_path.value(), error);
    }

    throw;
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    if (schema_path.has_value()) {
      throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
          schema_path.value());
    }

    throw;
  } catch (const sourcemeta::core::SchemaError &error) {
    if (schema_path.has_value()) {
      throw FileError<sourcemeta::core::SchemaError>(schema_path.value(),
                                                     error.what());
    }

    throw;
  }

  const auto schema_template{[&]() {
    try {
      return get_schema_template(bundled, *custom_resolver, frame, dialect,
                                 default_id, fast_mode, options);
    } catch (const sourcemeta::core::SchemaReferenceError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaReferenceError>(
            schema_path.value(), std::string{error.identifier()},
            error.location(), error.what());
      }

      throw;
    } catch (const sourcemeta::core::SchemaRelativeMetaschemaResolutionError
                 &error) {
      if (schema_path.has_value()) {
        throw FileError<
            sourcemeta::core::SchemaRelativeMetaschemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    } catch (const sourcemeta::core::SchemaResolutionError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaResolutionError>(
            schema_path.value(), error);
      }

      throw;
    } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaUnknownBaseDialectError>(
            schema_path.value());
      }

      throw;
    } catch (const sourcemeta::core::SchemaError &error) {
      if (schema_path.has_value()) {
        throw FileError<sourcemeta::core::SchemaError>(schema_path.value(),
                                                       error.what());
      }

      throw;
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

  if (trace && instance_arguments.size() > 1) {
    throw std::runtime_error{
        "The `--trace/-t` option is only allowed given a single instance"};
  }

  if (benchmark && instance_arguments.size() > 1) {
    throw std::runtime_error{
        "The `--benchmark/-b` option is only allowed given a single instance"};
  }

  for (const auto &instance_path_view : instance_arguments) {
    const std::filesystem::path instance_path{instance_path_view};
    if (trace && instance_path.extension() == ".jsonl") {
      throw std::runtime_error{
          "The `--trace/-t` option is only allowed given a single instance"};
    }

    if (trace && std::filesystem::is_directory(instance_path)) {
      throw std::runtime_error{
          "The `--trace/-t` option is only allowed given a single instance"};
    }

    if (benchmark && std::filesystem::is_directory(instance_path)) {
      throw std::runtime_error{"The `--benchmark/-b` option is only allowed "
                               "given a single instance"};
    }
    if (std::filesystem::is_directory(instance_path) ||
        instance_path.extension() == ".jsonl" ||
        instance_path.extension() == ".yaml" ||
        instance_path.extension() == ".yml") {
      for (const auto &entry : for_each_json({instance_path_view}, options)) {
        const auto &entry_path{entry.local_path_or_throw("validate")};
        std::ostringstream error;
        sourcemeta::blaze::SimpleOutput output{entry.second};
        bool subresult{true};
        if (benchmark) {
          subresult = run_loop(
              evaluator, schema_template, entry.second, entry_path,
              entry.multidocument ? static_cast<std::int64_t>(entry.index + 1)
                                  : static_cast<std::int64_t>(-1),
              benchmark_loop);
          if (!subresult) {
            error << "error: Schema validation failure\n";
            result = false;
          }
        } else if (fast_mode) {
          subresult = evaluator.validate(schema_template, entry.second);
        } else if (!json_output) {
          subresult = evaluator.validate(schema_template, entry.second,
                                         std::ref(output));
        }

        if (benchmark) {
          continue;
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
              break;
            }
          }
        } else if (subresult) {
          LOG_VERBOSE(options)
              << "ok: "
              << sourcemeta::core::weakly_canonical(entry_path).string();
          if (entry.multidocument) {
            LOG_VERBOSE(options) << " (entry #" << entry.index + 1 << ")";
          }
          LOG_VERBOSE(options) << "\n  matches " << schema_display << "\n";
          print_annotations(output, options, entry.positions, std::cerr);
        } else {
          std::cerr << "fail: "
                    << sourcemeta::core::weakly_canonical(entry_path).string();
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
    } else {
      sourcemeta::core::PointerPositionTracker tracker;
      const auto instance{sourcemeta::core::read_yaml_or_json(
          instance_path, std::ref(tracker))};
      std::ostringstream error;
      sourcemeta::blaze::SimpleOutput output{instance};
      sourcemeta::blaze::TraceOutput trace_output{
          sourcemeta::core::schema_walker, *custom_resolver,
          sourcemeta::core::empty_weak_pointer, frame};
      bool subresult{true};
      if (benchmark) {
        subresult = run_loop(evaluator, schema_template, instance,
                             instance_path, (int64_t)-1, benchmark_loop);
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
        print(trace_output, tracker, std::cout);
        result = subresult;
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
            << "\n  matches " << schema_display << "\n";
        print_annotations(output, options, tracker, std::cerr);
      } else {
        std::cerr << "fail: "
                  << sourcemeta::core::weakly_canonical(instance_path).string()
                  << "\n";
        std::cerr << error.str();
        print(output, tracker, std::cerr);
        result = false;
      }
    }
  }

  if (!result) {
    // Report a different exit code for validation failures, to
    // distinguish them from other errors
    throw Fail{2};
  }
}
