#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/output.h>
#include <sourcemeta/blaze/test.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonschema.h>
#include <sourcemeta/core/openapi.h>
#include <sourcemeta/core/parallel.h>
#include <sourcemeta/core/uri.h>

// The parallel module includes windows.h, which defines DELETE as a macro
// that would otherwise break parsing the HTTPMethod enumeration that the
// resolver transitively includes below
#if defined(_WIN32)
#undef DELETE
#endif

#include <algorithm> // std::min, std::max
#include <atomic>    // std::atomic
#include <chrono>    // std::chrono
#include <cstddef>   // std::size_t
#include <cstdint>   // std::uint8_t
#include <exception> // std::exception_ptr, std::current_exception, std::rethrow_exception
#include <iostream>    // std::cout
#include <map>         // std::map
#include <memory>      // std::unique_ptr, std::make_unique
#include <mutex>       // std::mutex, std::scoped_lock
#include <optional>    // std::optional
#include <sstream>     // std::ostringstream
#include <string>      // std::string
#include <string_view> // std::string_view
#include <thread>      // std::this_thread
#include <utility>     // std::unreachable, std::move
#include <vector>      // std::vector

#include "command.h"
#include "configuration.h"
#include "configure.h"
#include "error.h"
#include "input.h"
#include "logger.h"
#include "print.h"
#include "resolver.h"
#include "utils.h"

namespace {

using sourcemeta::core::TerminalStyle;
constexpr auto PASS_STYLE{TerminalStyle::Bold | TerminalStyle::Green};
constexpr auto FAIL_STYLE{TerminalStyle::Bold | TerminalStyle::Red};
constexpr auto EMPTY_STYLE{TerminalStyle::Bold | TerminalStyle::Yellow};
constexpr auto HEADING_STYLE{TerminalStyle::Bold | TerminalStyle::Cyan};

enum class TestStatus : std::uint8_t { Pass, Fail, NoTests };

auto format_status(TestStatus status) -> std::string {
  if (sourcemeta::core::terminal_color_enabled(
          sourcemeta::core::TerminalStream::Stdout)) {
    switch (status) {
      case TestStatus::Pass:
        return sourcemeta::jsonschema::paint(
            "✓ PASS", PASS_STYLE, sourcemeta::core::TerminalStream::Stdout);
      case TestStatus::Fail:
        return sourcemeta::jsonschema::paint(
            "✗ FAIL", FAIL_STYLE, sourcemeta::core::TerminalStream::Stdout);
      case TestStatus::NoTests:
        return sourcemeta::jsonschema::paint(
            "NO TESTS", EMPTY_STYLE, sourcemeta::core::TerminalStream::Stdout);
    }
    std::unreachable();
  }

  switch (status) {
    case TestStatus::Pass:
      return "PASS";
    case TestStatus::Fail:
      return "FAIL";
    case TestStatus::NoTests:
      return "NO TESTS";
  }
  std::unreachable();
}

auto format_error_label() -> std::string {
  return sourcemeta::jsonschema::paint(
      "error:", FAIL_STYLE, sourcemeta::core::TerminalStream::Stdout);
}

auto print_rdf_failure(const sourcemeta::jsonschema::InputJSON &entry,
                       const std::size_t test_index,
                       const sourcemeta::blaze::TestOutcome &outcome,
                       std::ostream &stream,
                       const std::string_view error_label = "error:") -> void {
  if (outcome.rdf_error.has_value()) {
    const auto &error{outcome.rdf_error.value()};
    auto position{entry.positions.get(
        sourcemeta::core::Pointer{"tests", test_index, "data"}.concat(
            error.instance_location))};
    if (!position.has_value()) {
      position = entry.positions.get(
          sourcemeta::core::Pointer{"tests", test_index, "dataPath"});
    }

    stream << error_label << " " << error.message << "\n";
    if (position.has_value()) {
      stream << "  at line " << std::get<0>(position.value()) << "\n";
      stream << "  at column " << std::get<1>(position.value()) << "\n";
    }

    stream << "  at instance location \""
           << sourcemeta::core::to_string(error.instance_location) << "\"\n";
    stream << "  at facet \"" << sourcemeta::jsonschema::facet_name(error.facet)
           << "\"\n";
    stream << "  at schema location " << error.schema_location << "\n";

    if (error.conflicting_schema_location.has_value()) {
      stream << "  at conflicting schema location "
             << error.conflicting_schema_location.value() << "\n";
    }

    if (error.inert_override_location.has_value()) {
      stream << "  at inert override location "
             << error.inert_override_location.value() << "\n";
    }

    stream << "  at file path " << entry.resolution_base.generic_string()
           << "\n";

    if (error.inert_override_location.has_value()) {
      stream << "\nThe x-jsonld-override mark was ignored because it does not "
                "enclose the\n";
      stream << "conflicting annotation. Move the conflicting annotation, or "
                "the reference\n";
      stream << "that brings it in, inside the overriding object for the "
                "override to\n";
      stream << "take effect\n";
    }
  } else {
    auto location{sourcemeta::core::Pointer{"tests", test_index, "rdf"}};
    auto position{entry.positions.get(location)};
    if (!position.has_value()) {
      location = sourcemeta::core::Pointer{"tests", test_index, "rdfPath"};
      position = entry.positions.get(location);
    }

    stream << error_label << " RDF expansion mismatch\n";
    if (position.has_value()) {
      stream << "  at line " << std::get<0>(position.value()) << "\n";
      stream << "  at column " << std::get<1>(position.value()) << "\n";
    }

    stream << "  at file path " << entry.resolution_base.generic_string()
           << "\n";
    stream << "  at location \"" << sourcemeta::core::to_string(location)
           << "\"\n\n";
    sourcemeta::core::prettify(outcome.rdf.value(), stream);
    stream << "\n";
  }
}

// A test document names its target either as a URI or as an array of them, so
// what gets reported back is however the document spelled it
auto format_target(const sourcemeta::core::JSON &document) -> std::string {
  const auto *target{document.try_at("target")};
  if (target == nullptr) {
    return {};
  }

  if (target->is_string()) {
    return target->to_string();
  }

  std::ostringstream result;
  sourcemeta::core::stringify(*target, result);
  return std::move(result).str();
}

// A test suite borrows the document and the frame that each of its targets
// resolved to, so whatever holds those must outlive it. Targets that share a
// base share the one entry, as all that sets them apart is where in it the
// schema under test sits
struct TestTargets {
  // A document is framed as whatever it is, and only one of the two frames is
  // ever built. A description holds many schemas and none of itself, so what
  // the suite is handed is the frame of the schemas within it
  struct Entry {
    std::unique_ptr<sourcemeta::core::JSON> document;
    std::unique_ptr<sourcemeta::core::SchemaFrame> schema;
    std::unique_ptr<sourcemeta::core::OpenAPIFrame> description;

    [[nodiscard]] auto frame() const -> const sourcemeta::core::SchemaFrame & {
      return this->schema == nullptr ? this->description->schemas()
                                     : *this->schema;
    }
  };

  std::map<sourcemeta::core::JSON::String, Entry> bases;
};

// The document was read, so what is reported is how it failed to answer to the
// target rather than anything about reaching it. Which of the three it is tells
// the user whether to look at the document, at the place the fragment names, or
// at the fragment being there at all
auto target_not_schema(const sourcemeta::core::JSON &document,
                       const bool description,
                       const sourcemeta::core::URI &target_uri,
                       const sourcemeta::core::JSON::String &target)
    -> sourcemeta::jsonschema::TestTargetNotSchemaError {
  if (!target_uri.fragment().has_value() ||
      target_uri.fragment().value().empty()) {
    return {"This target names a document rather than a schema within it",
            target, description, true};
  }

  const auto pointer{sourcemeta::core::fragment_to_pointer(target_uri)};
  if (pointer.has_value() &&
      sourcemeta::core::try_get(document, pointer.value()) != nullptr) {
    return {"This target names a place of the document that is not a schema",
            target, description, false};
  }

  return {"This target names a place that the document does not hold", target,
          description, false};
}

// What a target names is reached the same way whatever holds it, by bundling
// the document in and framing it under the base the target was resolved
// against. Everything it spans has to be bundled in first, as compiling never
// reaches for what a reference names
auto resolve_base(const sourcemeta::core::SchemaResolver &schema_resolver,
                  const sourcemeta::core::OpenAPIResolver &openapi_resolver,
                  sourcemeta::jsonschema::CustomResolver &custom_resolver,
                  const std::string &dialect,
                  const sourcemeta::core::JSON::String &base)
    -> TestTargets::Entry {
  // The two resolvers are strictly separated, so a description is never
  // reported as a schema and the other way around. Asking for a schema first
  // still sorts whatever a fetch turns up into the right one of the two, so a
  // description is read once however it got here
  const auto schema{schema_resolver(base)};
  if (schema.has_value()) {
    auto document{std::make_unique<sourcemeta::core::JSON>(
        sourcemeta::core::schema_bundle(
            schema.value(), sourcemeta::core::schema_walker, schema_resolver,
            dialect, base,
            sourcemeta::jsonschema::bundle_references_options()))};

    auto frame{std::make_unique<sourcemeta::core::SchemaFrame>(
        sourcemeta::core::SchemaFrame::Mode::References, *document,
        sourcemeta::core::schema_walker, schema_resolver, dialect, base)};

    return {.document = std::move(document),
            .schema = std::move(frame),
            .description = nullptr};
  }

  const auto description{openapi_resolver(base)};
  if (!description.has_value()) {
    // An imported description that declares this identifier within it is the
    // one case worth explaining, as the usual advice to import whatever names
    // it would have the user import what they already did
    custom_resolver.report_description_resource(base);
    throw sourcemeta::core::SchemaResolutionError{
        base, "Could not resolve the reference to an external schema"};
  }

  auto document{std::make_unique<sourcemeta::core::JSON>(description.value())};

  // A description that names itself with `$self` answers to that as well as to
  // where it was read from, and a target may spell either. What a relative
  // identity and relative references resolve against is where it was read
  // from, so that is the base rather than whichever of the two got us here
  const auto retrieval{custom_resolver.description_retrieval(base)};
  const auto &default_base{retrieval.has_value() ? retrieval.value() : base};

  sourcemeta::core::openapi_bundle(*document, sourcemeta::core::schema_walker,
                                   schema_resolver, openapi_resolver,
                                   {.default_base = default_base});

  auto frame{std::make_unique<sourcemeta::core::OpenAPIFrame>(
      *document, sourcemeta::core::schema_walker, schema_resolver,
      default_base)};

  return {.document = std::move(document),
          .schema = nullptr,
          .description = std::move(frame)};
}

auto resolve_test_target(
    const sourcemeta::core::SchemaResolver &schema_resolver,
    const sourcemeta::core::OpenAPIResolver &openapi_resolver,
    sourcemeta::jsonschema::CustomResolver &custom_resolver,
    const std::string &dialect, const sourcemeta::core::JSON::String &target,
    TestTargets &targets) -> sourcemeta::blaze::TestTarget {
  // RFC 3986 Section 3.5 makes a fragment address a place within a resource
  // rather than a resource of its own, so what the target names is looked up
  // under the base that precedes it
  const sourcemeta::core::URI target_uri{target};
  const auto base{target_uri.recompose_without_fragment().value_or(target)};

  auto match{targets.bases.find(base)};
  if (match == targets.bases.cend()) {
    match = targets.bases
                .emplace(base, resolve_base(schema_resolver, openapi_resolver,
                                            custom_resolver, dialect, base))
                .first;
  }

  const auto &entry{match->second};

  // Asking here reports a target that nothing locates in terms of the document
  // that was read for it, rather than as an entry point that whatever compiles
  // it next does not know how to talk about
  if (!entry.frame().traverse(target).has_value()) {
    throw target_not_schema(*entry.document, entry.schema == nullptr,
                            target_uri, target);
  }

  return {.document = *entry.document,
          .frame = entry.frame(),
          .entrypoint = target};
}

auto parse_test_suite(const sourcemeta::jsonschema::InputJSON &entry,
                      const sourcemeta::core::SchemaResolver &schema_resolver,
                      const sourcemeta::core::OpenAPIResolver &openapi_resolver,
                      sourcemeta::jsonschema::CustomResolver &custom_resolver,
                      const std::string &dialect,
                      const std::optional<sourcemeta::blaze::Tweaks> &tweaks,
                      TestTargets &targets) -> sourcemeta::blaze::TestSuite {
  try {
    return sourcemeta::blaze::TestSuite::parse(
        entry.second, entry.positions,
        // A test document read from standard input has no directory of its
        // own, and the base path must remain a real directory, as relative
        // `dataPath` and `rdfPath` entries are opened from it
        entry.from_stdin ? std::filesystem::current_path()
                         : entry.resolution_base.parent_path(),
        [&schema_resolver, &openapi_resolver, &custom_resolver, &dialect,
         &targets](const sourcemeta::core::JSON::String &target) {
          return resolve_test_target(schema_resolver, openapi_resolver,
                                     custom_resolver, dialect, target, targets);
        },
        schema_resolver, sourcemeta::core::schema_walker,
        sourcemeta::blaze::default_schema_compiler, tweaks);
  } catch (const sourcemeta::core::URIParseError &) {
    // Blaze parses every target as a URI, so the one it choked on is whichever
    // of them is not one. Reporting the lot keeps this from naming the wrong
    // target, as a document may list several
    throw sourcemeta::core::FileError<
        sourcemeta::jsonschema::InvalidTestTargetError>{
        entry.resolution_base, format_target(entry.second)};
  } catch (const sourcemeta::jsonschema::TestTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::jsonschema::TestTargetNotSchemaError>{entry.resolution_base,
                                                          error};
  } catch (const sourcemeta::core::OpenAPIResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::OpenAPIReferenceError &error) {
    // TODO: A reference of the description itself that names a plain schema
    // reports "This reference must name a place within the document it points
    // at", which never says that what it had to point at is another
    // description. The message belongs to Core, so sharpening it is upstream's
    // to do
    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIReferenceError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::OpenAPIError &error) {
    // No position, as what is bundled and framed here is the description the
    // document targets while the positions on hand describe the test document
    // itself
    throw sourcemeta::core::FileError<sourcemeta::core::OpenAPIError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::TestParseError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::TestParseError>{
        entry.resolution_base, error.what(), error.location(), error.line(),
        error.column()};
  } catch (
      const sourcemeta::blaze::CompilerReferenceTargetNotSchemaError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerReferenceTargetNotSchemaError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::blaze::CompilerInvalidRegexError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::blaze::CompilerInvalidRegexError>{entry.resolution_base,
                                                      error};
  } catch (const sourcemeta::blaze::CompilerError &error) {
    // No position, as what compiles here is the schema the document targets
    // while the positions on hand describe the test document itself
    throw sourcemeta::core::FileError<sourcemeta::blaze::CompilerError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::SchemaKeywordError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaKeywordError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::SchemaFrameError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaFrameError>{
        entry.resolution_base, error};
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaRelativeMetaschemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaResolutionError>{
        entry.resolution_base, error};
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownBaseDialectError>{entry.resolution_base};
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaVocabularyError>{
        entry.resolution_base, error.uri(), error.what()};
  } catch (const sourcemeta::core::SchemaUnknownDialectError &) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaUnknownDialectError>{entry.resolution_base};
  } catch (const sourcemeta::core::SchemaAnchorCollisionError &error) {
    // No position, as what compiles here is the schema the document targets
    // while the positions on hand describe the test document itself
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaAnchorCollisionError>{entry.resolution_base,
                                                      error};
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    // No position, as what compiles here is the schema the document targets
    // while the positions on hand describe the test document itself
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaReferenceError>{
        entry.resolution_base, error.identifier(), error.location(),
        error.what()};
  } catch (const sourcemeta::core::SchemaReferenceObjectResourceError &error) {
    throw sourcemeta::core::FileError<
        sourcemeta::core::SchemaReferenceObjectResourceError>{
        entry.resolution_base, error.identifier()};
  } catch (const sourcemeta::core::SchemaError &error) {
    throw sourcemeta::core::FileError<sourcemeta::core::SchemaError>{
        entry.resolution_base, error.what()};
  }
}

auto warm_caches(const sourcemeta::core::Options &options,
                 const std::vector<sourcemeta::jsonschema::InputJSON> &entries)
    -> void {
  for (const auto &entry : entries) {
    const auto configuration_path{sourcemeta::jsonschema::find_configuration(
        options, entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    [[maybe_unused]] const auto &schema_resolver{
        sourcemeta::jsonschema::resolver(options, options.contains("http"),
                                         dialect, configuration)};
  }
}

auto emit_target_header(
    const bool multi_target, const sourcemeta::core::JSON::String &target,
    std::optional<sourcemeta::core::JSON::String> &last_target_header,
    std::ostream &stream) -> void {
  if (multi_target && last_target_header != target) {
    stream << "  "
           << sourcemeta::jsonschema::paint(
                  target, HEADING_STYLE,
                  sourcemeta::core::TerminalStream::Stdout)
           << ":\n";
    last_target_header = target;
  }
}

auto run_suite_as_text(const sourcemeta::core::Options &options,
                       const sourcemeta::jsonschema::InputJSON &entry,
                       const bool verbose, std::ostream &stream)
    -> sourcemeta::blaze::TestSuite::Result {
  try {
    const auto configuration_path{sourcemeta::jsonschema::find_configuration(
        options, entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};
    const auto &openapi_resolver{sourcemeta::jsonschema::openapi_resolver(
        options, options.contains("http"), dialect, configuration)};
    auto &custom_resolver{sourcemeta::jsonschema::resolver_instance(
        options, options.contains("http"), dialect, configuration)};
    const auto trace{options.contains("trace")};

    TestTargets targets;
    auto test_suite{parse_test_suite(
        entry, schema_resolver, openapi_resolver, custom_resolver, dialect,
        sourcemeta::jsonschema::format_assertion_tweaks(options), targets)};

    stream << sourcemeta::jsonschema::paint(
                  entry.first, HEADING_STYLE,
                  sourcemeta::core::TerminalStream::Stdout)
           << ":";

    const auto multi_target{test_suite.targets.size() > 1};
    std::optional<sourcemeta::core::JSON::String> last_target_header;

    const auto suite_result{test_suite.run(
        [&](const sourcemeta::core::JSON::String &target,
            std::size_t target_index, std::size_t index, std::size_t total,
            const sourcemeta::blaze::TestCase &test_case,
            const sourcemeta::blaze::TestOutcome &outcome,
            sourcemeta::blaze::TestTimestamp,
            sourcemeta::blaze::TestTimestamp) {
          if (verbose && index == 1) {
            stream << "\n";
          }

          const auto *const entry_indent{multi_target ? "    " : "  "};

          const auto &description{test_case.description.empty()
                                      ? "<no description>"
                                      : test_case.description};

          if (outcome.passed) {
            if (verbose) {
              emit_target_header(multi_target, target, last_target_header,
                                 stream);
              stream << entry_indent << index << "/" << total << " "
                     << format_status(TestStatus::Pass) << " " << description
                     << "\n";
            }
          } else if (!test_case.valid && outcome.valid) {
            if (!verbose) {
              stream << "\n";
            }
            emit_target_header(multi_target, target, last_target_header,
                               stream);
            stream << entry_indent << index << "/" << total << " "
                   << format_status(TestStatus::Fail) << " " << description
                   << "\n\n"
                   << format_error_label()
                   << " Passed but was expected to fail\n";

            if (index != total && verbose) {
              stream << "\n";
            }
          } else if (!outcome.valid) {
            sourcemeta::blaze::SimpleOutput output{test_case.data};
            test_suite.evaluator.validate(test_suite.exhaustive(target_index),
                                          test_case.data, std::ref(output));

            if (!verbose) {
              stream << "\n";
            }
            emit_target_header(multi_target, target, last_target_header,
                               stream);
            stream << entry_indent << index << "/" << total << " "
                   << format_status(TestStatus::Fail) << " " << description
                   << "\n\n";
            sourcemeta::jsonschema::print(output, test_case.tracker, stream,
                                          format_error_label());

            if (trace) {
              stream << "\n";
              sourcemeta::blaze::TraceOutput trace_output{
                  test_suite.exhaustive(target_index),
                  sourcemeta::jsonschema::trace_callback(test_case.tracker,
                                                         stream)};
              test_suite.evaluator.validate(test_suite.exhaustive(target_index),
                                            test_case.data,
                                            std::ref(trace_output));
            }

            if (index != total && verbose) {
              stream << "\n";
            }
          } else {
            if (!verbose) {
              stream << "\n";
            }
            emit_target_header(multi_target, target, last_target_header,
                               stream);
            stream << entry_indent << index << "/" << total << " "
                   << format_status(TestStatus::Fail) << " " << description
                   << "\n\n";
            print_rdf_failure(entry, (index - 1) % test_suite.tests.size(),
                              outcome, stream, format_error_label());

            if (index != total && verbose) {
              stream << "\n";
            }
          }
        })};

    if (suite_result.total == 0) {
      stream << " " << format_status(TestStatus::NoTests) << "\n";
    } else if (!verbose && suite_result.passed == suite_result.total) {
      stream << " " << format_status(TestStatus::Pass) << " "
             << suite_result.passed << "/" << suite_result.total << "\n";
    }

    return suite_result;
  } catch (const sourcemeta::blaze::EvaluationError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::EvaluationError>(
        entry.resolution_base, error.what());
  }
}

auto report_as_text(const sourcemeta::core::Options &options,
                    const std::size_t jobs) -> void {
  bool result{true};
  bool empty_test_suite{false};
  const auto verbose{options.contains("verbose") || options.contains("debug")};

  const auto entries{sourcemeta::jsonschema::for_each_json(options)};
  warm_caches(options, entries);

  std::mutex output_mutex;
  std::atomic<bool> skip_remaining{false};
  std::exception_ptr first_error{nullptr};
  std::string first_error_path;

  sourcemeta::core::parallel_for_each(
      entries.cbegin(), entries.cend(),
      [&](const sourcemeta::jsonschema::InputJSON &entry, const std::size_t,
          const std::size_t) {
        if (skip_remaining.load()) {
          return;
        }

        try {
          // Buffer the output of every suite, so that we only need to hold
          // the output lock while emitting it, letting suites actually
          // evaluate their test cases in parallel
          std::ostringstream buffer;
          const auto suite_result{
              run_suite_as_text(options, entry, verbose, buffer)};

          const std::scoped_lock<std::mutex> lock{output_mutex};
          std::cout << buffer.str();

          if (suite_result.passed != suite_result.total) {
            result = false;
          }

          if (suite_result.total == 0) {
            empty_test_suite = true;
          }
        } catch (...) {
          const std::scoped_lock<std::mutex> lock{output_mutex};
          if (!first_error) {
            first_error = std::current_exception();
            first_error_path = entry.first;
            skip_remaining.store(true);
          }
        }
      },
      jobs);

  if (first_error) {
    std::cout << first_error_path << ":\n";
    std::rethrow_exception(first_error);
  }

  if (!result) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_EXPECTED_FAILURE};
  }

  // An empty test suite likely means the author forgot to write the tests,
  // so don't let it silently succeed
  if (empty_test_suite) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_OTHER_INPUT_ERROR};
  }
}

auto timestamp_to_unix_ms(
    const sourcemeta::blaze::TestTimestamp &timestamp,
    const std::chrono::system_clock::time_point &system_ref,
    const sourcemeta::blaze::TestTimestamp &steady_ref) -> std::int64_t {
  const auto offset{timestamp - steady_ref};
  const auto unix_time{system_ref + offset};
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             unix_time.time_since_epoch())
      .count();
}

auto duration_ms(const sourcemeta::blaze::TestTimestamp &start,
                 const sourcemeta::blaze::TestTimestamp &end) -> std::int64_t {
  return std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
      .count();
}

struct CtrfSuiteReport {
  std::vector<sourcemeta::core::JSON> tests;
  std::size_t passed{0};
  std::size_t total{0};
  sourcemeta::blaze::TestTimestamp start{};
  sourcemeta::blaze::TestTimestamp end{};
};

auto run_suite_as_ctrf(const sourcemeta::core::Options &options,
                       const sourcemeta::jsonschema::InputJSON &entry,
                       CtrfSuiteReport &report) -> void {
  try {
    const auto configuration_path{sourcemeta::jsonschema::find_configuration(
        options, entry.resolution_base)};
    const auto &configuration{sourcemeta::jsonschema::read_configuration(
        options, configuration_path)};
    const auto dialect{
        sourcemeta::jsonschema::default_dialect(options, configuration)};
    const auto &schema_resolver{sourcemeta::jsonschema::resolver(
        options, options.contains("http"), dialect, configuration)};
    const auto &openapi_resolver{sourcemeta::jsonschema::openapi_resolver(
        options, options.contains("http"), dialect, configuration)};
    auto &custom_resolver{sourcemeta::jsonschema::resolver_instance(
        options, options.contains("http"), dialect, configuration)};

    TestTargets targets;
    auto test_suite{parse_test_suite(
        entry, schema_resolver, openapi_resolver, custom_resolver, dialect,
        sourcemeta::jsonschema::format_assertion_tweaks(options), targets)};

    const auto file_path{entry.first};

    const auto suite_result{test_suite.run(
        [&](const sourcemeta::core::JSON::String &target,
            std::size_t target_index, std::size_t index, std::size_t,
            const sourcemeta::blaze::TestCase &test_case,
            const sourcemeta::blaze::TestOutcome &outcome,
            sourcemeta::blaze::TestTimestamp start,
            sourcemeta::blaze::TestTimestamp end) {
          auto test_object{sourcemeta::core::JSON::make_object()};

          const auto &name{test_case.description.empty()
                               ? "<no description>"
                               : test_case.description};
          test_object.assign("name", sourcemeta::core::JSON{name});

          test_object.assign(
              "status",
              sourcemeta::core::JSON{outcome.passed ? "passed" : "failed"});

          test_object.assign("duration",
                             sourcemeta::core::JSON{duration_ms(start, end)});
          auto suite{sourcemeta::core::JSON::make_array()};
          suite.push_back(sourcemeta::core::JSON{target});
          test_object.assign("suite", std::move(suite));
          test_object.assign("type", sourcemeta::core::JSON{"unit"});
          test_object.assign("filePath", sourcemeta::core::JSON{file_path});

          const auto [test_line, test_column, test_end_line, test_end_column] =
              test_case.position;
          test_object.assign("line", sourcemeta::core::JSON{
                                         static_cast<std::int64_t>(test_line)});
          test_object.assign(
              "retries", sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
          test_object.assign("flaky", sourcemeta::core::JSON{false});
          std::ostringstream thread_id_stream;
          thread_id_stream << std::this_thread::get_id();
          test_object.assign("threadId",
                             sourcemeta::core::JSON{thread_id_stream.str()});

          if (!outcome.passed) {
            if (!test_case.valid && outcome.valid) {
              test_object.assign("message",
                                 sourcemeta::core::JSON{"Passed but was "
                                                        "expected to fail"});
            } else if (!outcome.valid) {
              std::ostringstream trace_stream;
              sourcemeta::blaze::SimpleOutput output{test_case.data};
              test_suite.evaluator.validate(test_suite.exhaustive(target_index),
                                            test_case.data, std::ref(output));
              sourcemeta::jsonschema::print(output, test_case.tracker,
                                            trace_stream);
              test_object.assign("trace",
                                 sourcemeta::core::JSON{trace_stream.str()});
            } else {
              std::ostringstream trace_stream;
              print_rdf_failure(entry, (index - 1) % test_suite.tests.size(),
                                outcome, trace_stream);
              test_object.assign("trace",
                                 sourcemeta::core::JSON{trace_stream.str()});
            }
          }

          report.tests.push_back(std::move(test_object));
        })};

    report.passed = suite_result.passed;
    report.total = suite_result.total;
    report.start = suite_result.start;
    report.end = suite_result.end;
  } catch (const sourcemeta::blaze::EvaluationError &error) {
    throw sourcemeta::core::FileError<sourcemeta::blaze::EvaluationError>(
        entry.resolution_base, error.what());
  }
}

auto report_as_ctrf(const sourcemeta::core::Options &options,
                    const std::size_t jobs) -> void {
  bool result{true};
  bool empty_test_suite{false};

  const auto system_ref{std::chrono::system_clock::now()};
  const auto steady_ref{std::chrono::steady_clock::now()};

  const auto entries{sourcemeta::jsonschema::for_each_json(options)};
  warm_caches(options, entries);

  std::vector<CtrfSuiteReport> reports{entries.size()};
  std::mutex error_mutex;
  std::atomic<bool> skip_remaining{false};
  std::exception_ptr first_error{nullptr};

  sourcemeta::core::parallel_for_each(
      entries.cbegin(), entries.cend(),
      [&](const sourcemeta::jsonschema::InputJSON &entry, const std::size_t,
          const std::size_t) {
        if (skip_remaining.load()) {
          return;
        }

        try {
          run_suite_as_ctrf(
              options, entry,
              reports[static_cast<std::size_t>(&entry - entries.data())]);
        } catch (...) {
          const std::scoped_lock<std::mutex> lock{error_mutex};
          if (!first_error) {
            first_error = std::current_exception();
            skip_remaining.store(true);
          }
        }
      },
      jobs);

  if (first_error) {
    std::rethrow_exception(first_error);
  }

  auto ctrf_tests{sourcemeta::core::JSON::make_array()};
  std::size_t total_passed{0};
  std::size_t total_failed{0};
  sourcemeta::blaze::TestTimestamp global_start{};
  sourcemeta::blaze::TestTimestamp global_end{};
  bool first_suite{true};

  for (auto &report : reports) {
    if (first_suite) {
      global_start = report.start;
      global_end = report.end;
      first_suite = false;
    } else {
      global_start = std::min(global_start, report.start);
      global_end = std::max(global_end, report.end);
    }

    total_passed += report.passed;
    total_failed += report.total - report.passed;

    if (report.total == 0) {
      empty_test_suite = true;
    }

    if (report.passed != report.total) {
      result = false;
    }

    for (auto &test_object : report.tests) {
      ctrf_tests.push_back(std::move(test_object));
    }
  }

  // Build CTRF output
  auto summary{sourcemeta::core::JSON::make_object()};
  summary.assign("tests", sourcemeta::core::JSON{static_cast<std::int64_t>(
                              total_passed + total_failed)});
  summary.assign("passed", sourcemeta::core::JSON{
                               static_cast<std::int64_t>(total_passed)});
  summary.assign("failed", sourcemeta::core::JSON{
                               static_cast<std::int64_t>(total_failed)});
  summary.assign("pending",
                 sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("skipped",
                 sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("other", sourcemeta::core::JSON{static_cast<std::int64_t>(0)});
  summary.assign("start", sourcemeta::core::JSON{timestamp_to_unix_ms(
                              global_start, system_ref, steady_ref)});
  summary.assign("stop", sourcemeta::core::JSON{timestamp_to_unix_ms(
                             global_end, system_ref, steady_ref)});

  auto tool{sourcemeta::core::JSON::make_object()};
  tool.assign("name", sourcemeta::core::JSON{"jsonschema"});
  tool.assign("version",
              sourcemeta::core::JSON{sourcemeta::jsonschema::PROJECT_VERSION});

  auto results{sourcemeta::core::JSON::make_object()};
  results.assign("tool", std::move(tool));
  results.assign("summary", std::move(summary));
  results.assign("tests", std::move(ctrf_tests));

  auto ctrf{sourcemeta::core::JSON::make_object()};
  ctrf.assign("reportFormat", sourcemeta::core::JSON{"CTRF"});
  ctrf.assign("specVersion", sourcemeta::core::JSON{"0.0.0"});
  ctrf.assign("results", std::move(results));

  sourcemeta::core::prettify(ctrf, std::cout);
  std::cout << "\n";

  if (!result) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_EXPECTED_FAILURE};
  }

  // An empty test suite likely means the author forgot to write the tests,
  // so don't let it silently succeed
  if (empty_test_suite) {
    throw sourcemeta::jsonschema::Fail{
        sourcemeta::jsonschema::EXIT_OTHER_INPUT_ERROR};
  }
}

} // namespace

auto sourcemeta::jsonschema::test(const sourcemeta::core::Options &options)
    -> void {
  validate_http_headers(options);
  if (options.contains("trace") && options.contains("json")) {
    throw OptionConflictError{
        "The `--trace/-t` and `--json/-j` options are mutually exclusive"};
  }

  const auto jobs{parse_jobs(options)};
  LOG_VERBOSE(options) << "Using parallelism: " << jobs << "\n";
  if (options.contains("json")) {
    report_as_ctrf(options, jobs);
  } else {
    report_as_text(options, jobs);
  }
}
