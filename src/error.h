#ifndef SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_
#define SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>

#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <functional> // std::function
#include <stdexcept>  // std::runtime_error
#include <string>     // std::string

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

inline auto try_catch(const std::function<int()> &callback) noexcept -> int {
  try {
    return callback();
  } catch (const sourcemeta::jsonschema::PositionalArgumentError &error) {
    std::cerr << "error: " << error.what() << ". For example:\n\n"
              << "  " << error.example() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::NotSchemaError &error) {
    std::cerr << "error: " << error.what() << "\n  "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::YAMLInputError &error) {
    std::cerr << error.what() << "\n  "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::OptionConflictError &error) {
    std::cerr << "error: " << error.what() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::InvalidLintRuleError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "  " << error.rule() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::LintAutoFixError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    std::cerr << "  at schema location \""
              << sourcemeta::core::to_string(error.location()) << "\"\n\n";
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
  } catch (const sourcemeta::jsonschema::UnknownCommandError &error) {
    std::cerr << "error: " << error.what() << " '" << error.command() << "'\n";
    std::cerr << "Use '--help' for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.id()
              << "\n    at schema location \"";
    sourcemeta::core::stringify(error.location(), std::cerr);
    std::cerr << "\"\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::FileError<
           sourcemeta::core::SchemaConfigParseError> &error) {
    std::cerr << "error: " << error.what() << "\n  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    std::cerr << "  at location \""
              << sourcemeta::core::to_string(error.location()) << "\"\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::FileError<
           sourcemeta::core::SchemaRelativeMetaschemaResolutionError> &error) {
    std::cerr << "error: " << error.what() << "\n  uri " << error.id() << "\n";
    std::cerr << "  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (
      const sourcemeta::core::SchemaRelativeMetaschemaResolutionError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.id() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::FileError<
           sourcemeta::core::SchemaResolutionError> &error) {
    std::cerr << "error: " << error.what() << "\n  uri " << error.id() << "\n";
    std::cerr << "  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";

    if (error.id().starts_with("file://")) {
      std::cerr << "\nThis is likely because the file does not exist\n";
    } else {
      std::cerr << "\nThis is likely because you forgot to import such schema "
                   "using --resolve/-r\n";
    }

    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaResolutionError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.id() << "\n";

    if (error.id().starts_with("file://")) {
      std::cerr << "\nThis is likely because the file does not exist\n";
    } else {
      std::cerr << "\nThis is likely because you forgot to import such schema "
                   "using --resolve/-r\n";
    }

    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownDialectError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr
        << "\nThis is likely because you forgot to import such meta-schema "
           "using --resolve/-r\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::FileError<
           sourcemeta::core::SchemaUnknownBaseDialectError> &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr << "If the input does not declare the $schema keyword, you might "
                 "want to\n";
    std::cerr
        << "explicitly declare a default dialect using --default-dialect/-d\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaUnknownBaseDialectError &error) {
    std::cerr << "error: " << error.what() << "\n";
    std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                 "base dialect is known?\n";
    std::cerr << "If the input does not declare the $schema keyword, you might "
                 "want to\n";
    std::cerr
        << "explicitly declare a default dialect using --default-dialect/-d\n";
    return EXIT_FAILURE;
  } catch (
      const sourcemeta::jsonschema::FileError<sourcemeta::core::SchemaError>
          &error) {
    std::cerr << "error: " << error.what() << "\n  at "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaError &error) {
    std::cerr << "error: " << error.what() << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::SchemaVocabularyError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.uri()
              << "\n\nTo request support for it, please open an issue "
                 "at\nhttps://github.com/sourcemeta/jsonschema\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::URIParseError &error) {
    std::cerr << "error: " << error.what() << " at column " << error.column()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONFileParseError &error) {
    std::cerr << "error: " << error.what() << " at line " << error.line()
              << " and column " << error.column() << "\n  "
              << sourcemeta::core::weakly_canonical(error.path()).string()
              << "\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::JSONParseError &error) {
    std::cerr << "error: " << error.what() << " at line " << error.line()
              << " and column " << error.column() << "\n";
    return EXIT_FAILURE;
  } catch (const std::filesystem::filesystem_error &error) {
    // See https://en.cppreference.com/w/cpp/error/errc
    if (error.code() == std::errc::no_such_file_or_directory) {
      std::cerr << "error: " << error.code().message() << "\n  "
                << sourcemeta::core::weakly_canonical(error.path1()).string()
                << "\n";
    } else if (error.code() == std::errc::is_a_directory) {
      std::cerr << "error: The input was supposed to be a file but it is a "
                   "directory\n  "
                << sourcemeta::core::weakly_canonical(error.path1()).string()
                << "\n";
    } else {
      std::cerr << "error: " << error.what() << "\n";
    }

    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsUnexpectedValueFlagError &error) {
    std::cerr << "error: " << error.what() << " '" << error.name() << "'\n";
    std::cerr << "Use '--help' for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsMissingOptionValueError &error) {
    std::cerr << "error: " << error.what() << " '" << error.name() << "'\n";
    std::cerr << "Use '--help' for usage information\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::core::OptionsUnknownOptionError &error) {
    std::cerr << "error: " << error.what() << " '" << error.name() << "'\n";
    std::cerr << "Use '--help' for usage information\n";
    return EXIT_FAILURE;
  } catch (const std::runtime_error &error) {
    std::cerr << "error: " << error.what() << "\n";
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
