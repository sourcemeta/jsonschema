#include <sourcemeta/core/options.h>
#include <sourcemeta/core/stacktrace.h>
#include <sourcemeta/core/test.h>

#include <cstddef>         // std::size_t
#include <cstdlib>         // EXIT_SUCCESS, EXIT_FAILURE
#include <exception>       // std::exception
#include <filesystem>      // std::filesystem::path
#include <functional>      // std::function
#include <iostream>        // std::cout
#include <source_location> // std::source_location
#include <string>          // std::string
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

auto print_diagnostic(std::string_view message) -> void {
  std::size_t start{0};
  while (start <= message.size()) {
    const auto newline{message.find('\n', start)};
    const auto end{newline == std::string_view::npos ? message.size()
                                                     : newline};
    std::cout << "# " << message.substr(start, end - start) << "\n";
    if (newline == std::string_view::npos) {
      break;
    }

    start = newline + 1;
  }
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
  static constexpr std::string_view suffix{"_test"};
  if (stem.size() > suffix.size() && stem.ends_with(suffix)) {
    stem.erase(stem.size() - suffix.size());
  }

  return stem;
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
