#include "commands.h"

#include <algorithm> // std::min
#include <cstdlib>   // EXIT_FAILURE
#include <exception> // std::exception
#include <iostream>  // std::cerr
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <vector>    // std::vector

static auto assert_arguments(const std::string &command,
                             const std::vector<std::string> &arguments,
                             const std::vector<std::string>::size_type count)
    -> void {
  if (arguments.size() >= count) {
    return;
  }

  std::ostringstream error{};
  error << "Command '" << command << "' requires " << count
        << " arguments, but you passed " << arguments.size() << ".";
  throw std::runtime_error(error.str());
}

static auto cli_main(const std::string &program, const std::string &command,
                     const std::vector<std::string> &arguments) -> int {
  if (command.empty()) {
    sourcemeta::jsonbinpack::cli::help(program);
    return EXIT_FAILURE;
  }

  if (command == "help") {
    return sourcemeta::jsonbinpack::cli::help(program);
  }

  if (command == "version") {
    return sourcemeta::jsonbinpack::cli::version();
  }

  if (command == "canonicalize") {
    if (arguments.empty()) {
      return sourcemeta::jsonbinpack::cli::canonicalize();
    }

    assert_arguments(command, arguments, 1);
    return sourcemeta::jsonbinpack::cli::canonicalize(arguments.at(0));
  }

  if (command == "compile") {
    if (arguments.empty()) {
      return sourcemeta::jsonbinpack::cli::compile();
    }

    assert_arguments(command, arguments, 1);
    return sourcemeta::jsonbinpack::cli::compile(arguments.at(0));
  }

  if (command == "encode") {
    if (arguments.size() == 1) {
      return sourcemeta::jsonbinpack::cli::encode(arguments.at(0));
    }

    assert_arguments(command, arguments, 2);
    return sourcemeta::jsonbinpack::cli::encode(arguments.at(0),
                                                arguments.at(1));
  }

  if (command == "decode") {
    if (arguments.size() == 1) {
      return sourcemeta::jsonbinpack::cli::decode(arguments.at(0));
    }

    assert_arguments(command, arguments, 2);
    return sourcemeta::jsonbinpack::cli::decode(arguments.at(0),
                                                arguments.at(1));
  }

  std::cerr << "Unknown command: " << command << "\n";
  return EXIT_FAILURE;
}

auto main(int argc, char *argv[]) -> int {
  const std::string program{argv[0]};
  const std::string command{argc > 1 ? argv[1] : ""};
  const std::vector<std::string> arguments{argv + std::min(2, argc),
                                           argv + argc};

  try {
    return cli_main(program, command, arguments);
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << "\n";
    return EXIT_FAILURE;
  }
}
