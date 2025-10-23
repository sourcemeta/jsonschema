#ifndef SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_
#define SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>

#include <cassert>      // assert
#include <cstdlib>      // EXIT_FAILURE
#include <filesystem>   // std::filesystem
#include <functional>   // std::function
#include <iostream>     // std::cout, std::cerr
#include <optional>     // std::optional
#include <stdexcept>    // std::runtime_error
#include <string>       // std::string
#include <system_error> // std::errc
#include <type_traits>  // std::is_base_of_v

namespace sourcemeta::jsonschema {

class PositionalArgumentError : public std::runtime_error {
public:
  PositionalArgumentError(std::string message, std::string example)
      : std::runtime_error{message}, example_{std::move(example)} {}

  [[nodiscard]] auto example() const noexcept -> const std::string & {
    return this->example_;
  }

private:
  std::string example_;
};

class NotSchemaError : public std::runtime_error {
public:
  NotSchemaError(std::filesystem::path path)
      : std::runtime_error{"The schema file you provided does not represent a "
                           "valid JSON "
                           "Schema"},
        path_{std::move(path)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

class YAMLInputError : public std::runtime_error {
public:
  YAMLInputError(std::string message, std::filesystem::path path)
      : std::runtime_error{std::move(message)}, path_{std::move(path)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

class OptionConflictError : public std::runtime_error {
public:
  OptionConflictError(std::string message)
      : std::runtime_error{std::move(message)} {}
};

class InvalidLintRuleError : public std::runtime_error {
public:
  InvalidLintRuleError(std::string message, std::string rule)
      : std::runtime_error{std::move(message)}, rule_{std::move(rule)} {}

  [[nodiscard]] auto rule() const noexcept -> const std::string & {
    return this->rule_;
  }

private:
  std::string rule_;
};

class LintAutoFixError : public std::runtime_error {
public:
  LintAutoFixError(std::string message, std::filesystem::path path,
                   sourcemeta::core::Pointer location)
      : std::runtime_error{std::move(message)}, path_{std::move(path)},
        location_{std::move(location)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

private:
  std::filesystem::path path_;
  sourcemeta::core::Pointer location_;
};

class UnknownCommandError : public std::runtime_error {
public:
  UnknownCommandError(std::string command)
      : std::runtime_error{"Unknown command"}, command_{std::move(command)} {}

  [[nodiscard]] auto command() const noexcept -> const std::string & {
    return this->command_;
  }

private:
  std::string command_;
};

class TestError : public std::runtime_error {
public:
  TestError(std::string message, std::optional<unsigned int> test_number)
      : std::runtime_error{std::move(message)},
        test_number_{std::move(test_number)} {}

  [[nodiscard]] auto test_number() const noexcept
      -> const std::optional<unsigned int> & {
    return this->test_number_;
  }

private:
  std::optional<unsigned int> test_number_;
};

class Fail : public std::runtime_error {
public:
  Fail(int exit_code) : std::runtime_error{"Fail"}, exit_code_{exit_code} {}

  [[nodiscard]] auto exit_code() const noexcept -> int {
    return this->exit_code_;
  }

private:
  int exit_code_;
};

template <typename T> class FileError : public T {
public:
  template <typename... Args>
  FileError(std::filesystem::path path, Args &&...args)
      : T{std::forward<Args>(args)...}, path_{std::move(path)} {
    assert(std::filesystem::exists(this->path_));
  }

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return path_;
  }

private:
  std::filesystem::path path_;
};

template <typename Exception>
inline auto print_exception(const Exception &exception) -> void {
  if constexpr (std::is_base_of_v<std::filesystem::filesystem_error,
                                  Exception>) {
    if (exception.code() == std::errc::no_such_file_or_directory) {
      std::cerr << "error: " << exception.code().message() << "\n";
    } else if (exception.code() == std::errc::is_a_directory) {
      std::cerr << "error: The input was supposed to be a file but it is a "
                   "directory\n";
    } else {
      std::cerr << "error: " << exception.what() << "\n";
    }
  } else {
    std::cerr << "error: " << exception.what() << "\n";
  }

  if constexpr (requires(const Exception &current) { current.id(); }) {
    std::cerr << "  at identifier " << exception.id() << "\n";
  }

  if constexpr (requires(const Exception &current) { current.test_number(); }) {
    if (exception.test_number().has_value()) {
      std::cerr << "  at test case #" << exception.test_number().value()
                << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.line(); }) {
    std::cerr << "  at line " << exception.line() << "\n";
  }

  if constexpr (requires(const Exception &current) { current.column(); }) {
    std::cerr << "  at column " << exception.column() << "\n";
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.path()
                  } -> std::convertible_to<std::filesystem::path>;
                }) {
    std::cerr << "  at file path "
              << sourcemeta::core::weakly_canonical(exception.path()).string()
              << "\n";
  } else if constexpr (requires(const Exception &current) {
                         {
                           current.path1()
                         } -> std::convertible_to<std::filesystem::path>;
                       }) {
    std::cerr << "  at file path "
              << sourcemeta::core::weakly_canonical(exception.path1()).string()
              << "\n";
  }

  if constexpr (requires(const Exception &current) { current.location(); }) {
    std::cerr << "  at location \""
              << sourcemeta::core::to_string(exception.location()) << "\"\n";
  }

  if constexpr (requires(const Exception &current) { current.uri(); }) {
    std::cerr << "  at uri " << exception.uri() << "\n";
  }

  if constexpr (requires(const Exception &current) {
                  { current.rule() } -> std::convertible_to<std::string>;
                }) {
    std::cerr << "  at rule " << exception.rule() << "\n";
  }

  if constexpr (requires(const Exception &current) {
                  { current.command() } -> std::convertible_to<std::string>;
                }) {
    std::cerr << "  at command " << exception.command() << "\n";
  }

  if constexpr (requires(const Exception &current) {
                  { current.option() } -> std::convertible_to<std::string>;
                }) {
    std::cerr << "  at option " << exception.option() << "\n";
  }
}

inline auto try_catch(const std::function<int()> &callback) noexcept -> int {
  try {
    return callback();
  } catch (const Fail &error) {
    return error.exit_code();
  } catch (const NotSchemaError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const YAMLInputError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const InvalidLintRuleError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const LintAutoFixError &error) {
    print_exception(error);
    std::cerr << "\n";
    std::cerr << "This is an unexpected error, as making the auto-fix "
                 "functionality work in all\n";
    std::cerr << "cases is tricky. We are working hard to improve the "
                 "auto-fixing functionality\n";
    std::cerr << "to handle all possible edge cases, but for now, try again "
                 "without `--fix/-f`\n";
    std::cerr << "and apply the suggestions by hand.\n\n";
    std::cerr << "Also consider consider reporting this problematic case to "
                 "the issue tracker,\n";
    std::cerr << "so we can add it to the test suite and fix it:\n\n";
    std::cerr << "https://github.com/sourcemeta/jsonschema/issues\n";
    return EXIT_FAILURE;
  } catch (const TestError &error) {
    print_exception(error);
    std::cerr << "\n";
    std::cerr << "Learn more here: "
                 "https://github.com/sourcemeta/jsonschema/blob/main/"
                 "docs/test.markdown\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const FileError<sourcemeta::core::SchemaConfigParseError> &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (
      const FileError<sourcemeta::core::SchemaRelativeMetaschemaResolutionError>
          &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const FileError<sourcemeta::core::SchemaResolutionError> &error) {
    print_exception(error);

    if (error.id().starts_with("file://")) {
      std::cerr << "\nThis is likely because the file does not exist\n";
    } else {
      std::cerr << "\nThis is likely because you forgot to import such schema "
                   "using `--resolve/-r`\n";
    }

    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    print_exception(error);

    if (error.id().starts_with("file://")) {
      std::cerr << "\nThis is likely because the file does not exist\n";
    } else {
      std::cerr << "\nThis is likely because you forgot to import such schema "
                   "using `--resolve/-r`\n";
    }

    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownDialectError &error) {
    print_exception(error);
    std::cerr
        << "\nThis is likely because you forgot to import such meta-schema "
           "using `--resolve/-r`\n";
    return EXIT_FAILURE;
  } catch (
      const FileError<sourcemeta::core::SchemaUnknownBaseDialectError> &error) {
    print_exception(error);
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr
        << "If the input does not declare the `$schema` keyword, you might "
           "want to\n";
    std::cerr << "explicitly declare a default dialect using "
                 "`--default-dialect/-d`\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &error) {
    print_exception(error);
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr
        << "If the input does not declare the `$schema` keyword, you might "
           "want to\n";
    std::cerr << "explicitly declare a default dialect using "
                 "`--default-dialect/-d`\n";
    return EXIT_FAILURE;
  } catch (const FileError<sourcemeta::core::SchemaError> &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    print_exception(error);
    std::cerr << "\nTo request support for it, please open an issue "
                 "at\nhttps://github.com/sourcemeta/jsonschema\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::URIParseError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONFileParseError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONParseError &error) {
    print_exception(error);
    return EXIT_FAILURE;

    // Command line parsing handling
  } catch (const OptionConflictError &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const PositionalArgumentError &error) {
    print_exception(error);
    std::cerr << "\nFor example: " << error.example() << "\n";
    return EXIT_FAILURE;
  } catch (const UnknownCommandError &error) {
    print_exception(error);
    std::cerr << "\nRun the `help` command for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsUnexpectedValueFlagError &error) {
    print_exception(error);
    std::cerr << "\nRun the `help` command for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsMissingOptionValueError &error) {
    print_exception(error);
    std::cerr << "\nRun the `help` command for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsUnknownOptionError &error) {
    print_exception(error);
    std::cerr << "\nRun the `help` command for usage information\n";
    return EXIT_FAILURE;

    // Standard library handlers
  } catch (const std::filesystem::filesystem_error &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const std::runtime_error &error) {
    print_exception(error);
    return EXIT_FAILURE;
  } catch (const std::exception &error) {
    std::cerr << "unexpected error: " << error.what()
              << "\nPlease report it at "
              << "https://github.com/sourcemeta/jsonschema\n";
    return EXIT_FAILURE;
  }
}

} // namespace sourcemeta::jsonschema

#endif
