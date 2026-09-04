#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/stacktrace.h>
#include <sourcemeta/core/test.h>
#include <sourcemeta/core/text.h>

#include <cstddef>         // std::size_t
#include <cstdlib>         // EXIT_SUCCESS, EXIT_FAILURE
#include <exception>       // std::exception
#include <filesystem>      // std::filesystem::path
#include <functional>      // std::function
#include <iostream>        // std::cout
#include <source_location> // std::source_location
#include <sstream>         // std::ostringstream
#include <string>          // std::string
#include <string_view>     // std::string_view
#include <utility>         // std::move
#include <vector>          // std::vector

namespace {

struct RegisteredTest {
  std::string suite;
  std::string name;
  std::string_view file;
  int line;
  std::function<void()> body;
};

auto registry() -> std::vector<RegisteredTest> & {
  static std::vector<RegisteredTest> tests;
  return tests;
}

auto base_name(std::string_view path) -> std::string {
  return std::filesystem::path{path}.filename().string();
}

auto print_usage(std::string_view program) -> void {
  std::cout
      << "Usage: " << std::filesystem::path{program}.stem().string()
      << " [options]\n\n"
      << "Run the registered tests.\n\n"
      << "Options:\n"
      << "  -f, --filter <prefix>  Only run tests whose <suite>.<name> starts "
         "with <prefix>\n"
      << "  -h, --help             Show this message\n";
}

auto has_line_terminator(const std::string_view value) -> bool {
  return value.contains('\n');
}

auto print_diagnostic(std::string_view message) -> void {
  sourcemeta::core::split(message, '\n',
                          [](const std::string_view line) -> void {
                            std::cout << "# " << line << "\n";
                          });
}

} // namespace

namespace sourcemeta::core {

auto test_register(std::string_view suite, std::string_view name,
                   std::string_view file, int line, std::function<void()> body)
    -> int {
  registry().push_back({.suite = std::string{suite},
                        .name = std::string{name},
                        .file = file,
                        .line = line,
                        .body = std::move(body)});
  return 0;
}

auto test_register(std::string_view name, std::function<void()> body,
                   std::source_location location) -> int {
  return test_register(test_suite_from_path(location.file_name()), name,
                       location.file_name(), static_cast<int>(location.line()),
                       std::move(body));
}

[[noreturn]] auto test_report_failure(std::string_view file, int line,
                                      std::string_view message) -> void {
  throw TestAbortError{base_name(file) + ":" + std::to_string(line) + ": " +
                       std::string{message}};
}

auto test_suite_from_path(std::string_view path) -> std::string {
  std::string stem{std::filesystem::path{path}.stem().string()};
  static constexpr std::string_view SUFFIX{"_test"};
  if (stem.size() > SUFFIX.size() && stem.ends_with(SUFFIX)) {
    stem.erase(stem.size() - SUFFIX.size());
  }

  return stem;
}

auto test_stringify_difference(std::string_view actual,
                               std::string_view expected) -> std::string {
  // Values that hold a single line speak for themselves, and an unexpected
  // equality has no difference to show at all
  if (actual == expected ||
      (!has_line_terminator(actual) && !has_line_terminator(expected))) {
    return {};
  }

  const auto result{
      diff(actual, expected, Diff::Mode::Line, Diff::Algorithm::Myers)};
  std::ostringstream stream;
  stringify(result, stream, Diff::Format::Unified,
            {.original_label = "actual", .modified_label = "expected"});
  auto output{stream.str()};
  // The last line of a rendered difference is terminated, whereas the message
  // this becomes part of is not
  if (!output.empty() && output.back() == '\n') {
    output.pop_back();
  }

  return output;
}

auto test_run(int argc, char **argv) -> int {
  sourcemeta::core::stacktrace_on_crash();

  sourcemeta::core::Options options;
  options.option("filter", {"f"});
  options.flag("help", {"h"});

  try {
    options.parse(argc, argv);
  } catch (const sourcemeta::core::OptionsError &) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  if (options.contains("help")) {
    print_usage(argv[0]);
    // A test binary exists to run tests, so an explicit help request exits with
    // failure on purpose. This keeps a stray help invocation from being
    // mistaken for a passing test
    return EXIT_FAILURE;
  }

  std::string_view needle;
  if (options.contains("filter") && !options.at("filter").empty()) {
    needle = options.at("filter").front();
  }

  std::vector<const RegisteredTest *> selected;
  for (const auto &entry : registry()) {
    const std::string identifier{entry.suite + "." + entry.name};
    if (needle.empty() || identifier.starts_with(needle)) {
      selected.push_back(&entry);
    }
  }

  std::cout << "TAP version 14\n";
  std::cout << "1.." << selected.size() << "\n";

  // A test binary exists to run tests, so selecting none exits with failure on
  // purpose. An empty run means either broken registration or a filter that
  // matches nothing, neither of which may pass as a green suite
  if (selected.empty()) {
    if (needle.empty()) {
      print_diagnostic("no tests were registered");
    } else {
      print_diagnostic("no tests matched the filter: " + std::string{needle});
    }

    return EXIT_FAILURE;
  }

  std::size_t number{0};
  std::size_t passed{0};
  std::size_t failed{0};

  for (const auto *entry : selected) {
    number += 1;
    const std::string identifier{entry->suite + "." + entry->name};

    try {
      entry->body();
      std::cout << "ok " << number << " - " << identifier << "\n";
      passed += 1;
    } catch (const TestAbortError &error) {
      std::cout << "not ok " << number << " - " << identifier << "\n";
      print_diagnostic(error.message);
      failed += 1;
    } catch (const std::exception &error) {
      std::cout << "not ok " << number << " - " << identifier << "\n";
      print_diagnostic(base_name(entry->file) + ":" +
                       std::to_string(entry->line) +
                       ": threw an unexpected exception: " + error.what());
      failed += 1;
    } catch (...) {
      std::cout << "not ok " << number << " - " << identifier << "\n";
      print_diagnostic(base_name(entry->file) + ":" +
                       std::to_string(entry->line) +
                       ": threw an unexpected unknown exception");
      failed += 1;
    }
  }

  std::cout << "# " << passed << " passed, " << failed << " failed\n";
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // namespace sourcemeta::core
