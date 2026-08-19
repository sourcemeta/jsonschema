#include "clitest.h"
#include "help.h"

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/text.h>

#include <cstdlib>    // EXIT_FAILURE, EXIT_SUCCESS
#include <exception>  // std::exception
#include <filesystem> // std::filesystem::temp_directory_path
#include <iostream>   // std::cout
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include <vector>     // std::vector

// Every diagnostic is worded here rather than where it was raised, as only this
// far along is it known what the reader is going to be shown
static auto describe(const std::string_view script, const std::size_t line,
                     const std::string_view message,
                     const std::string_view context) -> std::string {
  std::ostringstream stream;
  stream << script << ":" << line << ": " << message;
  if (!context.empty()) {
    // A difference or a listing spans lines of its own, and reads better under
    // the message than trailing off the end of it
    stream << (!context.contains('\n') ? ": " : ":\n")
           << (context.ends_with('\n') ? context.substr(0, context.size() - 1)
                                       : context);
  }

  return stream.str();
}

static auto describe(const std::string_view script,
                     const sourcemeta::core::CLITestProblem &problem)
    -> std::string {
  return describe(script, problem.line, problem.message, problem.context);
}

static auto describe(const std::string_view script,
                     const sourcemeta::core::CLITestError &error)
    -> std::string {
  auto result{describe(script, error.line(), error.what(), error.context())};
  if (error.code()) {
    result.append(" (").append(error.code().message()).append(")");
  }

  return result;
}

// Everything the program said is not a context in the `name: value` sense, but
// supplementary output, so it follows the message rather than qualifying it
static auto describe(const std::string_view script,
                     const sourcemeta::core::CLITestExitError &error)
    -> std::string {
  std::ostringstream stream;
  stream << script << ":" << error.line() << ": " << error.what();
  if (error.actual().has_value()) {
    stream << ": expected " << error.expected() << ", got "
           << error.actual().value();
  }

  const std::string_view observation{error.observation()};
  if (!observation.empty()) {
    stream << "\n"
           << (observation.ends_with('\n')
                   ? observation.substr(0, observation.size() - 1)
                   : observation);
  }

  return stream.str();
}

// One script, in a temporary directory of its own. Whether it held up, with
// whatever it had to say collected rather than printed, as a test point is
// announced ahead of its own diagnostics
static auto run(const std::string &script, const bool check,
                const std::string &binary,
                const sourcemeta::core::CLITestBindings &bindings,
                std::vector<std::string> &notes) -> bool {
  std::string contents;
  try {
    auto stream{sourcemeta::core::read_file(script)};
    // A script checked out on a platform that spells a line ending with a
    // carriage return is the same script, so the runner takes it rather than
    // asking every consumer to configure their checkout. Beyond that a line
    // ends at a line feed and nothing else, which is what keeps a script
    // meaning the same thing everywhere
    contents = sourcemeta::core::replace(
        sourcemeta::core::read_to_string(stream), "\r\n", "\n");
  } catch (const sourcemeta::core::IOFileNotFoundError &) {
    notes.emplace_back(script + ": no such file");
    return false;
  } catch (const sourcemeta::core::IOIsADirectoryError &) {
    notes.emplace_back(script + ": is a directory");
    return false;
  } catch (const sourcemeta::core::IOFilePermissionError &) {
    notes.emplace_back(script + ": cannot be read");
    return false;
  } catch (const std::exception &error) {
    notes.emplace_back(script + ": " + error.what());
    return false;
  }

  const auto lines{sourcemeta::core::split(contents, '\n')};

  std::vector<sourcemeta::core::CLITestStatement> statements;
  try {
    statements = sourcemeta::core::clitest_parse(lines);
  } catch (const sourcemeta::core::CLITestError &error) {
    notes.push_back(describe(script, error));
    return false;
  }

  const auto problems{sourcemeta::core::clitest_check(statements, bindings)};
  if (!problems.empty()) {
    for (const auto &problem : problems) {
      notes.push_back(describe(script, problem));
    }

    return false;
  }

  if (check) {
    return true;
  }

  // Fully resolved, as a platform that reaches its temporary directory through
  // a symbolic link would otherwise name it differently from the way the
  // program under test prints it back
  const sourcemeta::core::TemporaryDirectory sandbox{
      std::filesystem::temp_directory_path(), "clitest-"};
  auto environment{sourcemeta::core::clitest_environment(
      sourcemeta::core::canonical(sandbox.path()), bindings)};

  try {
    sourcemeta::core::clitest_interpret(statements, binary, environment);
  } catch (const sourcemeta::core::CLITestError &error) {
    notes.push_back(describe(script, error));
    return false;
  } catch (const sourcemeta::core::CLITestExitError &error) {
    notes.push_back(describe(script, error));
    return false;
  } catch (const std::exception &error) {
    // Whatever went wrong, it went wrong in one script. Letting it escape would
    // abandon the run without a test point for this script or a final count
    notes.emplace_back(script + ": " + error.what());
    return false;
  }

  return true;
}

auto main(int argc, char *argv[]) -> int {
  sourcemeta::core::Options application;
  application.option("binary", {"b"});
  application.option("environment", {"e"});
  application.flag("check", {"c"});
  application.flag("help", {"h"});
  application.parse(argc, argv);

  if (application.contains("help") || application.positional().empty()) {
    std::cout << sourcemeta::core::CLITEST_HELP;
    return application.contains("help") ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  const auto check{application.contains("check")};
  if (!check && !application.contains("binary")) {
    std::cout << "TAP version 14\n"
                 "Bail out! no program to run, pass --binary or --check\n";
    return EXIT_FAILURE;
  }

  sourcemeta::core::CLITestBindings bindings;
  for (const auto entry : application.at("environment")) {
    const auto separator{entry.find('=')};
    if (separator == 0) {
      std::cout << "TAP version 14\n"
                   "Bail out! a binding needs a name before its value\n";
      return EXIT_FAILURE;
    }
    if (separator == std::string_view::npos) {
      bindings.insert_or_assign(std::string{entry}, "");
    } else {
      bindings.insert_or_assign(std::string{entry.substr(0, separator)},
                                std::string{entry.substr(separator + 1)});
    }
  }

  const std::string binary{
      check ? "" : std::string{application.at("binary").front()}};

  std::cout << "TAP version 14\n";
  std::cout << "1.." << application.positional().size() << "\n";

  // Every script is judged, so that one failing early does not hide what the
  // rest of them would have said
  std::size_t number{0};
  std::size_t passed{0};
  for (const auto script : application.positional()) {
    number += 1;
    std::vector<std::string> notes;
    if (run(std::string{script}, check, binary, bindings, notes)) {
      passed += 1;
      std::cout << "ok " << number << " - " << script << "\n";
    } else {
      std::cout << "not ok " << number << " - " << script << "\n";
    }

    // Anything that is not a test point is a comment, including the lines of a
    // difference or of everything a program said
    for (const auto &note : notes) {
      sourcemeta::core::split(note, '\n',
                              [](const std::string_view line) -> void {
                                std::cout << "# " << line << "\n";
                              });
    }
  }

  std::cout << "# " << passed << " passed, " << number - passed << " failed\n";
  return passed == number ? EXIT_SUCCESS : EXIT_FAILURE;
}
