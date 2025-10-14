#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonl.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/yaml.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>
#include <sourcemeta/blaze/output.h>

#include <chrono>   // std::chrono
#include <cmath>    // std::sqrt
#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cerr
#include <string>   // std::string

#include "command.h"
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
                         const std::optional<std::string> &default_dialect,
                         const std::optional<std::string> &default_id,
                         const bool fast_mode,
                         const sourcemeta::core::Options &options)
    -> sourcemeta::blaze::Template {
  const auto precompiled{get_precompiled_schema_template_path(options)};
  if (precompiled.has_value()) {
    sourcemeta::jsonschema::cli::log_verbose(options)
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
      std::cerr << "warning: Failed to parse pre-compiled schema template. "
                   "Compiling from scratch\n";
    }
  }

  return sourcemeta::blaze::compile(
      bundled, sourcemeta::core::schema_official_walker, resolver,
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

auto sourcemeta::jsonschema::cli::validate(
    const sourcemeta::core::Options &options) -> int {
  if (options.positional().size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema and a path to an\n"
        << "instance to validate against the schema. For example:\n\n"
        << "  jsonschema validate path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  if (options.positional().size() < 2) {
    std::cerr
        << "error: In addition to the schema, you must also pass an argument\n"
        << "that represents the instance to validate against. For example:\n\n"
        << "  jsonschema validate path/to/schema.json path/to/instance.json\n";
    return EXIT_FAILURE;
  }

  const auto &schema_path{options.positional().at(0)};
  const auto configuration_path{find_configuration(schema_path)};
  const auto &configuration{read_configuration(options, configuration_path)};
  const auto dialect{default_dialect(options, configuration)};
  const auto &custom_resolver{
      resolver(options, options.contains("http"), dialect, configuration)};

  const auto schema{sourcemeta::core::read_yaml_or_json(schema_path)};

  if (!sourcemeta::core::is_schema(schema)) {
    std::cerr << "error: The schema file you provided does not represent a "
                 "valid JSON Schema\n  "
              << sourcemeta::core::weakly_canonical(schema_path).string()
              << "\n";
    return EXIT_FAILURE;
  }

  const auto fast_mode{options.contains("fast")};
  const auto benchmark{options.contains("benchmark")};
  const auto benchmark_loop{parse_loop(options)};
  if (benchmark_loop == 0) {
    throw std::runtime_error("The loop number cannot be zero");
  }

  const auto trace{options.contains("trace")};
  const auto json_output{options.contains("json")};

  const auto default_id{sourcemeta::core::URI::from_path(
                            sourcemeta::core::weakly_canonical(schema_path))
                            .recompose()};
  const sourcemeta::core::JSON bundled{
      sourcemeta::core::bundle(schema, sourcemeta::core::schema_official_walker,
                               custom_resolver, dialect, default_id)};
  sourcemeta::core::SchemaFrame frame{
      sourcemeta::core::SchemaFrame::Mode::References};
  frame.analyse(bundled, sourcemeta::core::schema_official_walker,
                custom_resolver, dialect, default_id);
  const auto schema_template{get_schema_template(bundled, custom_resolver,
                                                 frame, dialect, default_id,
                                                 fast_mode, options)};

  sourcemeta::blaze::Evaluator evaluator;

  bool result{true};

  auto iterator{options.positional().cbegin()};
  std::advance(iterator, 1);
  for (; iterator != options.positional().cend(); ++iterator) {
    const std::filesystem::path instance_path{*iterator};
    if (instance_path.extension() == ".jsonl") {
      log_verbose(options)
          << "Interpreting input as JSONL: "
          << sourcemeta::core::weakly_canonical(instance_path).string() << "\n";
      std::int64_t index{0};
      auto stream{sourcemeta::core::read_file(instance_path)};
      try {
        for (const auto &instance : sourcemeta::core::JSONL{stream}) {
          index += 1;
          std::ostringstream error;
          // TODO: Get real positions for JSONL
          sourcemeta::core::PointerPositionTracker tracker;
          sourcemeta::blaze::SimpleOutput output{instance};
          sourcemeta::blaze::TraceOutput trace_output{
              sourcemeta::core::schema_official_walker, custom_resolver,
              sourcemeta::core::empty_weak_pointer, frame};
          bool subresult = true;
          if (benchmark) {
            if (benchmark_loop > 1)
              subresult = run_loop(evaluator, schema_template, instance,
                                   instance_path, index, benchmark_loop);
            else {
              const auto timestamp_start{
                  std::chrono::high_resolution_clock::now()};
              subresult = evaluator.validate(schema_template, instance);
              const auto timestamp_end{
                  std::chrono::high_resolution_clock::now()};
              const auto duration_us{
                  std::chrono::duration_cast<std::chrono::microseconds>(
                      timestamp_end - timestamp_start)};
              if (subresult) {
                std::cout << "took: " << duration_us.count() << "us\n";
              } else {
                error << "error: Schema validation failure\n";
              }
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
            // TODO: Get instance positions for JSONL too
            const auto suboutput{sourcemeta::blaze::standard(
                evaluator, schema_template, instance,
                fast_mode ? sourcemeta::blaze::StandardOutput::Flag
                          : sourcemeta::blaze::StandardOutput::Basic)};
            assert(suboutput.is_object());
            assert(suboutput.defines("valid"));
            assert(suboutput.at("valid").is_boolean());

            sourcemeta::core::prettify(suboutput, std::cout);
            std::cout << "\n";

            if (!suboutput.at("valid").to_boolean()) {
              result = false;
              break;
            }
          } else if (subresult) {
            log_verbose(options)
                << "ok: "
                << sourcemeta::core::weakly_canonical(instance_path).string()
                << " (entry #" << index << ")"
                << "\n  matches "
                << sourcemeta::core::weakly_canonical(schema_path).string()
                << "\n";
            print_annotations(output, options, tracker, std::cerr);
          } else {
            std::cerr
                << "fail: "
                << sourcemeta::core::weakly_canonical(instance_path).string()
                << " (entry #" << index << ")\n\n";
            sourcemeta::core::prettify(instance, std::cerr);
            std::cerr << "\n\n";
            std::cerr << error.str();
            print(output, tracker, std::cerr);
            result = false;
            break;
          }
        }
      } catch (const sourcemeta::core::JSONParseError &error) {
        // For producing better error messages
        throw sourcemeta::core::JSONFileParseError(instance_path, error);
      }

      if (index == 0) {
        log_verbose(options) << "warning: The JSONL file is empty\n";
      }
    } else {
      sourcemeta::core::PointerPositionTracker tracker;
      const auto instance{sourcemeta::core::read_yaml_or_json(
          instance_path, std::ref(tracker))};
      std::ostringstream error;
      sourcemeta::blaze::SimpleOutput output{instance};
      sourcemeta::blaze::TraceOutput trace_output{
          sourcemeta::core::schema_official_walker, custom_resolver,
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
        log_verbose(options)
            << "ok: "
            << sourcemeta::core::weakly_canonical(instance_path).string()
            << "\n  matches "
            << sourcemeta::core::weakly_canonical(schema_path).string() << "\n";
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

  return result ? EXIT_SUCCESS
                // Report a different exit code for validation failures, to
                // distinguish them from other errors
                : 2;
}
