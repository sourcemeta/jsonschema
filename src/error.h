#ifndef SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_
#define SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_

#include <sourcemeta/blaze/alterschema.h>
#include <sourcemeta/blaze/codegen.h>
#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/blaze/foundation.h>
#include <sourcemeta/blaze/frame.h>
#include <sourcemeta/blaze/test.h>
#include <sourcemeta/core/error.h>
#include <sourcemeta/core/gzip.h>
#include <sourcemeta/core/http.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonld.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/yaml.h>

#include <concepts>         // std::same_as, std::convertible_to
#include <cstdint>          // std::uint64_t
#include <filesystem>       // std::filesystem
#include <functional>       // std::function
#include <initializer_list> // std::initializer_list
#include <iostream>         // std::cout, std::cerr
#include <stdexcept>        // std::runtime_error
#include <string>           // std::string
#include <type_traits>      // std::is_base_of_v, std::is_same_v
#include <utility>          // std::forward, std::move
#include <vector>           // std::vector

#include "exit_code.h"

namespace sourcemeta::jsonschema {

template <typename T> class PositionError : public T {
public:
  template <typename... Args>
  PositionError(const std::uint64_t line, const std::uint64_t column,
                Args &&...args)
      : T{std::forward<Args>(args)...}, line_{line}, column_{column} {}

  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::uint64_t line_;
  std::uint64_t column_;
};

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

class InvalidOptionEnumerationValueError : public std::runtime_error {
public:
  InvalidOptionEnumerationValueError(std::string message, std::string option,
                                     std::initializer_list<std::string> values)
      : std::runtime_error{message}, option_{std::move(option)},
        values_{values} {}

  [[nodiscard]] auto option() const noexcept -> const std::string & {
    return this->option_;
  }

  [[nodiscard]] auto values() const noexcept
      -> const std::vector<std::string> & {
    return this->values_;
  }

private:
  std::string option_;
  std::vector<std::string> values_;
};

class InvalidDefaultDialectError : public std::runtime_error {
public:
  InvalidDefaultDialectError(std::string value)
      : std::runtime_error{"The default dialect is not a valid URI reference"},
        value_{std::move(value)} {}

  [[nodiscard]] auto value() const noexcept -> const std::string & {
    return this->value_;
  }

private:
  std::string value_;
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

class StdinError : public std::runtime_error {
public:
  explicit StdinError(std::string message)
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

class DuplicateLintRuleError : public std::runtime_error {
public:
  DuplicateLintRuleError(std::string rule)
      : std::runtime_error{"A lint rule with this name already exists"},
        rule_{std::move(rule)} {}

  [[nodiscard]] auto rule() const noexcept -> const std::string & {
    return this->rule_;
  }

private:
  std::string rule_;
};

class InvalidIncludeIdentifier : public std::runtime_error {
public:
  InvalidIncludeIdentifier(std::string identifier)
      : std::runtime_error{"The include identifier is not a valid C/C++ "
                           "identifier"},
        identifier_{std::move(identifier)} {}

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->identifier_;
  }

private:
  std::string identifier_;
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

class CustomMetaschemaUpgradeError : public std::runtime_error {
public:
  CustomMetaschemaUpgradeError(std::filesystem::path path,
                               sourcemeta::core::Pointer location,
                               std::string dialect)
      : std::runtime_error{"Cannot upgrade a schema that uses a custom "
                           "meta-schema"},
        path_{std::move(path)}, location_{std::move(location)},
        dialect_{std::move(dialect)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto uri() const noexcept -> const std::string & {
    return this->dialect_;
  }

private:
  std::filesystem::path path_;
  sourcemeta::core::Pointer location_;
  std::string dialect_;
};

class RdfResolutionError : public std::runtime_error {
public:
  RdfResolutionError(std::string message, std::string facet,
                     sourcemeta::core::Pointer instance_location,
                     std::filesystem::path path)
      : std::runtime_error{std::move(message)}, facet_{std::move(facet)},
        instance_location_{std::move(instance_location)},
        path_{std::move(path)} {}

  [[nodiscard]] auto facet() const noexcept -> const std::string & {
    return this->facet_;
  }

  [[nodiscard]] auto instance_location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->instance_location_;
  }

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::string facet_;
  sourcemeta::core::Pointer instance_location_;
  std::filesystem::path path_;
};

class UnsupportedDialectRdfError : public std::runtime_error {
public:
  UnsupportedDialectRdfError(std::filesystem::path path, std::string dialect)
      : std::runtime_error{"This command requires the schema to declare JSON "
                           "Schema 2019-09 or newer"},
        path_{std::move(path)}, dialect_{std::move(dialect)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

  [[nodiscard]] auto identifier() const noexcept -> const std::string & {
    return this->dialect_;
  }

private:
  std::filesystem::path path_;
  std::string dialect_;
};

class UnsupportedDialectUpgradeError : public std::runtime_error {
public:
  UnsupportedDialectUpgradeError(std::filesystem::path path,
                                 sourcemeta::core::Pointer location,
                                 std::string dialect)
      : std::runtime_error{"Upgrading schemas from this dialect is not "
                           "supported yet"},
        path_{std::move(path)}, location_{std::move(location)},
        dialect_{std::move(dialect)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto uri() const noexcept -> const std::string & {
    return this->dialect_;
  }

private:
  std::filesystem::path path_;
  sourcemeta::core::Pointer location_;
  std::string dialect_;
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

class ConfigurationNotFoundError : public std::runtime_error {
public:
  ConfigurationNotFoundError(std::filesystem::path path)
      : std::runtime_error{"Could not find a jsonschema.json configuration "
                           "file"},
        path_{std::move(path)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

class LockNotFoundError : public std::runtime_error {
public:
  LockNotFoundError(std::filesystem::path path)
      : std::runtime_error{"Lock file not found"}, path_{std::move(path)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

class LockParseError : public std::runtime_error {
public:
  LockParseError(std::filesystem::path path)
      : std::runtime_error{"Lock file is corrupted"}, path_{std::move(path)} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->path_;
  }

private:
  std::filesystem::path path_;
};

class InstallError : public std::runtime_error {
public:
  InstallError(std::string message, std::string uri)
      : std::runtime_error{std::move(message)}, uri_{std::move(uri)} {}

  [[nodiscard]] auto uri() const noexcept -> const std::string & {
    return this->uri_;
  }

private:
  std::string uri_;
};

class ConfigurationResolveFileNotFoundError : public std::runtime_error {
public:
  ConfigurationResolveFileNotFoundError(
      std::filesystem::path configuration_path,
      sourcemeta::core::Pointer location, std::filesystem::path resolve_path,
      std::uint64_t line, std::uint64_t column)
      : std::runtime_error{"The resolve target does not exist on the "
                           "filesystem"},
        configuration_path_{std::move(configuration_path)},
        location_{std::move(location)}, resolve_path_{std::move(resolve_path)},
        line_{line}, column_{column} {}

  [[nodiscard]] auto path() const noexcept -> const std::filesystem::path & {
    return this->configuration_path_;
  }

  [[nodiscard]] auto location() const noexcept
      -> const sourcemeta::core::Pointer & {
    return this->location_;
  }

  [[nodiscard]] auto resolve_path() const noexcept
      -> const std::filesystem::path & {
    return this->resolve_path_;
  }

  [[nodiscard]] auto line() const noexcept -> std::uint64_t {
    return this->line_;
  }

  [[nodiscard]] auto column() const noexcept -> std::uint64_t {
    return this->column_;
  }

private:
  std::filesystem::path configuration_path_;
  sourcemeta::core::Pointer location_;
  std::filesystem::path resolve_path_;
  std::uint64_t line_;
  std::uint64_t column_;
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

inline auto stdin_path() -> std::filesystem::path {
#ifdef _WIN32
  return std::filesystem::path{"<stdin>"};
#else
  return std::filesystem::path{"/dev/stdin"};
#endif
}

inline auto stdin_path_string(const std::filesystem::path &p) -> std::string {
#ifdef _WIN32
  if (p.string() == "<stdin>") {
    return "<stdin>";
  }
#else
  if (p == std::filesystem::path{"/dev/stdin"}) {
    return "/dev/stdin";
  }
#endif
  return sourcemeta::core::weakly_canonical(p).string();
}

template <typename Exception>
inline auto print_exception(const bool is_json, const Exception &exception)
    -> void {
  auto error_json{sourcemeta::core::JSON::make_object()};

  if constexpr (std::is_same_v<Exception,
                               sourcemeta::core::IOFileNotFoundError>) {
    if (is_json) {
      error_json.assign("error",
                        sourcemeta::core::JSON{"No such file or directory"});
    } else {
      std::cerr << "error: No such file or directory\n";
    }
  } else if constexpr (std::is_same_v<Exception,
                                      sourcemeta::core::IOIsADirectoryError>) {
    if (is_json) {
      error_json.assign(
          "error",
          sourcemeta::core::JSON{
              "The input was supposed to be a file but it is a directory"});
    } else {
      std::cerr << "error: The input was supposed to be a file but it is a "
                   "directory\n";
    }
  } else if constexpr (std::is_base_of_v<std::filesystem::filesystem_error,
                                         Exception>) {
    if (is_json) {
      error_json.assign("error", sourcemeta::core::JSON{exception.what()});
    } else {
      std::cerr << "error: " << exception.what() << "\n";
    }
  } else {
    if (is_json) {
      error_json.assign("error", sourcemeta::core::JSON{exception.what()});
    } else {
      std::cerr << "error: " << exception.what() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.identifier(); }) {
    if (is_json) {
      error_json.assign("identifier",
                        sourcemeta::core::JSON{exception.identifier()});
    } else {
      std::cerr << "  at identifier " << exception.identifier() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.value(); }) {
    if (is_json) {
      error_json.assign("value", sourcemeta::core::JSON{exception.value()});
    } else {
      std::cerr << "  at value " << exception.value() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.keyword()
                  } -> std::convertible_to<std::string_view>;
                }) {
    if (is_json) {
      error_json.assign("keyword", sourcemeta::core::JSON{exception.keyword()});
    } else {
      std::cerr << "  at keyword " << exception.keyword() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.resolve_path()
                  } -> std::convertible_to<std::filesystem::path>;
                }) {
    const auto &resolve_path_value{exception.resolve_path()};
    if (is_json) {
      error_json.assign("resolvePath",
                        sourcemeta::core::JSON{resolve_path_value.string()});
    } else {
      std::cerr << "  at resolve path " << resolve_path_value.string() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.line(); }) {
    if (is_json) {
      error_json.assign("line", sourcemeta::core::JSON{static_cast<std::size_t>(
                                    exception.line())});
    } else {
      std::cerr << "  at line " << exception.line() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.column(); }) {
    if (is_json) {
      error_json.assign(
          "column",
          sourcemeta::core::JSON{static_cast<std::size_t>(exception.column())});
    } else {
      std::cerr << "  at column " << exception.column() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.regex(); }) {
    if (is_json) {
      error_json.assign("regex", sourcemeta::core::JSON{exception.regex()});
    } else {
      std::cerr << "  at regex " << exception.regex() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.instance_location()
                  } -> std::convertible_to<const sourcemeta::core::Pointer &>;
                }) {
    if (is_json) {
      error_json.assign("instanceLocation",
                        sourcemeta::core::JSON{sourcemeta::core::to_string(
                            exception.instance_location())});
    } else {
      std::cerr << "  at instance location \""
                << sourcemeta::core::to_string(exception.instance_location())
                << "\"\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.facet()
                  } -> std::convertible_to<const std::string &>;
                }) {
    if (is_json) {
      error_json.assign("facet", sourcemeta::core::JSON{exception.facet()});
    } else {
      std::cerr << "  at facet \"" << exception.facet() << "\"\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.path()
                  } -> std::convertible_to<std::filesystem::path>;
                }) {
    const auto &error_path{exception.path()};
    const auto error_path_string{stdin_path_string(error_path)};
    if (is_json) {
      error_json.assign("filePath", sourcemeta::core::JSON{error_path_string});
    } else {
      std::cerr << "  at file path " << error_path_string << "\n";
    }
  } else if constexpr (requires(const Exception &current) {
                         {
                           current.path1()
                         } -> std::convertible_to<std::filesystem::path>;
                       }) {
    if (is_json) {
      error_json.assign(
          "filePath",
          sourcemeta::core::JSON{
              sourcemeta::core::weakly_canonical(exception.path1()).string()});
    } else {
      std::cerr
          << "  at file path "
          << sourcemeta::core::weakly_canonical(exception.path1()).string()
          << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.location(); }) {
    if (is_json) {
      error_json.assign("location",
                        sourcemeta::core::JSON{
                            sourcemeta::core::to_string(exception.location())});
    } else {
      std::cerr << "  at location \""
                << sourcemeta::core::to_string(exception.location()) << "\"\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.pointer()
                  } -> std::convertible_to<const sourcemeta::core::Pointer &>;
                }) {
    if (is_json) {
      error_json.assign("location",
                        sourcemeta::core::JSON{
                            sourcemeta::core::to_string(exception.pointer())});
    } else {
      std::cerr << "  at document location \""
                << sourcemeta::core::to_string(exception.pointer()) << "\"\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.other()
                  } -> std::convertible_to<const sourcemeta::core::Pointer &>;
                }) {
    if (is_json) {
      error_json.assign("otherLocation",
                        sourcemeta::core::JSON{
                            sourcemeta::core::to_string(exception.other())});
    } else {
      std::cerr << "  at other location \""
                << sourcemeta::core::to_string(exception.other()) << "\"\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.status()
                  } -> std::same_as<sourcemeta::core::HTTPStatus>;
                }) {
    const auto status{exception.status()};
    if (is_json) {
      error_json.assign("status", sourcemeta::core::JSON{
                                      static_cast<std::size_t>(status.code)});
    } else {
      std::cerr << "  with status ";
      if (status.wire.empty()) {
        std::cerr << status.code;
      } else {
        std::cerr << status.wire;
      }
      std::cerr << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.method()
                  } -> std::same_as<sourcemeta::core::HTTPMethod>;
                }) {
    if (is_json) {
      error_json.assign(
          "method",
          sourcemeta::core::JSON{std::string{
              sourcemeta::core::http_method_string(exception.method())}});
    } else {
      std::cerr << "  with method "
                << sourcemeta::core::http_method_string(exception.method())
                << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  { current.url() } -> std::convertible_to<std::string_view>;
                }) {
    if (is_json) {
      error_json.assign("url",
                        sourcemeta::core::JSON{std::string{exception.url()}});
    } else {
      std::cerr << "  at url " << exception.url() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  current.base().recompose();
                }) {
    if (is_json) {
      error_json.assign("baseURI",
                        sourcemeta::core::JSON{exception.base().recompose()});
    } else {
      std::cerr << "  at base uri " << exception.base().recompose() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) { current.uri(); }) {
    if (is_json) {
      error_json.assign("uri", sourcemeta::core::JSON{exception.uri()});
    } else {
      std::cerr << "  at uri " << exception.uri() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  { current.rule() } -> std::convertible_to<std::string>;
                }) {
    if (is_json) {
      error_json.assign("rule", sourcemeta::core::JSON{exception.rule()});
    } else {
      std::cerr << "  at rule " << exception.rule() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  { current.command() } -> std::convertible_to<std::string>;
                }) {
    if (is_json) {
      error_json.assign("command", sourcemeta::core::JSON{exception.command()});
    } else {
      std::cerr << "  at command " << exception.command() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  { current.option() } -> std::convertible_to<std::string_view>;
                }) {
    if (is_json) {
      error_json.assign("option", sourcemeta::core::JSON{exception.option()});
    } else {
      std::cerr << "  at option " << exception.option() << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.values()
                  } -> std::convertible_to<const std::vector<std::string> &>;
                }) {
    if (is_json) {
      auto values_array{sourcemeta::core::JSON::make_array()};
      for (const auto &value : exception.values()) {
        values_array.push_back(sourcemeta::core::JSON{value});
      }
      error_json.assign("values", std::move(values_array));
    } else {
      std::cerr << "  with values\n";
      for (const auto &value : exception.values()) {
        std::cerr << "  - " << value << "\n";
      }
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.variable()
                  } -> std::convertible_to<std::string_view>;
                }) {
    if (is_json) {
      error_json.assign("environmentVariable",
                        sourcemeta::core::JSON{exception.variable()});
    } else {
      std::cerr << "  with environment variable " << exception.variable()
                << "\n";
    }
  }

  if constexpr (requires(const Exception &current) {
                  {
                    current.paths()
                  } -> std::convertible_to<const std::vector<std::string> &>;
                }) {
    if (is_json) {
      auto paths_array{sourcemeta::core::JSON::make_array()};
      for (const auto &path : exception.paths()) {
        paths_array.push_back(sourcemeta::core::JSON{path});
      }
      error_json.assign("paths", std::move(paths_array));
    } else {
      std::cerr << "  with paths\n";
      for (const auto &path : exception.paths()) {
        std::cerr << "  - " << path << "\n";
      }
    }
  }

  if (is_json) {
    sourcemeta::core::prettify(error_json, std::cout);
    std::cout << "\n";
  }
}

inline auto try_catch(const sourcemeta::core::Options &options,
                      const std::function<int()> &callback) noexcept -> int {
  try {
    return callback();
  } catch (const Fail &error) {
    return error.exit_code();
  } catch (const InstallError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const ConfigurationNotFoundError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nLearn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/install.markdown\n";
    }

    return EXIT_OTHER_INPUT_ERROR;
  } catch (const LockNotFoundError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const LockParseError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const NotSchemaError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const YAMLInputError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const PositionError<UnsupportedDialectUpgradeError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const UnsupportedDialectUpgradeError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const PositionError<RdfResolutionError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const RdfResolutionError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const UnsupportedDialectRdfError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nThe x-jsonld-* keywords rely on annotation collection, "
                   "which JSON Schema\n";
      std::cerr << "only introduced in the 2019-09 dialect. Consider running "
                   "the `upgrade`\n";
      std::cerr << "command to move your schema to a newer dialect\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const InvalidLintRuleError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const sourcemeta::core::FileError<DuplicateLintRuleError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaRuleInvalidNameError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaRuleInvalidNamePatternError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaRuleMissingNameError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const InvalidIncludeIdentifier &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const LintAutoFixError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
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
    }

    return EXIT_UNEXPECTED_ERROR;
  } catch (const PositionError<CustomMetaschemaUpgradeError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\n";
      std::cerr << "Schemas that declare a custom meta-schema cannot be "
                   "upgraded in place\n";
      std::cerr << "by this command. Please upgrade the meta-schema and the "
                   "schema manually.\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const CustomMetaschemaUpgradeError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\n";
      std::cerr << "Schemas that declare a custom meta-schema cannot be "
                   "upgraded in place\n";
      std::cerr << "by this command. Please upgrade the meta-schema and the "
                   "schema manually.\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<sourcemeta::blaze::TestParseError>
               &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\n";
      std::cerr << "Learn more here: "
                   "https://github.com/sourcemeta/jsonschema/blob/main/"
                   "docs/test.markdown\n";
    }

    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CompilerReferenceTargetNotSchemaError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\n";

      if (!error.location().empty() && error.location().back().is_property() &&
          error.location().back().to_property() == "$defs") {
        std::cerr << "Maybe you meant to use `definitions` instead of `$defs` "
                     "in this dialect?\n";
      } else {
        std::cerr << "Are you sure the reported location is a valid JSON "
                     "Schema keyword in this dialect?\n";
      }
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CompilerInvalidEntryPoint> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr
          << "\nUse the `inspect` command to find valid schema locations\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CompilerInvalidRegexError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nDetailed regex error messages are not yet supported\n"
                   "Try tools like https://regex101.com to debug further\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (
      const sourcemeta::core::FileError<sourcemeta::blaze::SchemaReferenceError>
          &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const ConfigurationResolveFileNotFoundError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::ConfigurationParseError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (
      const sourcemeta::core::FileError<sourcemeta::core::JSONLDError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaRelativeMetaschemaResolutionError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaResolutionError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      if (error.identifier().starts_with("file://")) {
        std::cerr << "\nThis is likely because the file does not exist\n";
      } else {
        std::cerr
            << "\nThis is likely because you forgot to import such schema "
               "using `--resolve/-r`\n";
      }
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaUnknownBaseDialectError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                   "base dialect is known?\n";
      std::cerr
          << "If the input does not declare the `$schema` keyword, you might "
             "want to\n";
      std::cerr << "explicitly declare a default dialect using "
                   "`--default-dialect/-d`\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaUnknownDialectError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nAre you sure the input is a valid JSON Schema and its "
                   "dialect is known?\n";
      std::cerr
          << "If the input does not declare the `$schema` keyword, you might "
             "want to\n";
      std::cerr << "explicitly declare a default dialect using "
                   "`--default-dialect/-d`\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (
      const sourcemeta::core::FileError<sourcemeta::blaze::SchemaKeywordError>
          &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nAre you sure the input is a valid JSON Schema and it is "
                   "valid according to its meta-schema?\n";
    }

    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const PositionError<sourcemeta::core::FileError<
               sourcemeta::blaze::SchemaAnchorCollisionError>> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaAnchorCollisionError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (
      const sourcemeta::core::FileError<sourcemeta::blaze::SchemaFrameError>
          &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaReferenceObjectResourceError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<sourcemeta::blaze::SchemaError>
               &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::SchemaVocabularyError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_SCHEMA_INPUT_ERROR;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CodegenUnsupportedKeywordError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CodegenUnsupportedKeywordValueError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const sourcemeta::core::FileError<
           sourcemeta::blaze::CodegenUnexpectedSchemaError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;
  } catch (const sourcemeta::core::YAMLFileParseError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::JSONFileParseError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::JSONParseError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;

    // Command line parsing handling
  } catch (const StdinError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const OptionConflictError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const InvalidOptionEnumerationValueError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nRun the `help` command for usage information\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (
      const sourcemeta::core::FileError<InvalidDefaultDialectError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const InvalidDefaultDialectError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const PositionalArgumentError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nFor example: " << error.example() << "\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const UnknownCommandError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nRun the `help` command for usage information\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const sourcemeta::core::OptionsUnexpectedValueFlagError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nRun the `help` command for usage information\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const sourcemeta::core::OptionsMissingOptionValueError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nRun the `help` command for usage information\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;
  } catch (const sourcemeta::core::OptionsUnknownOptionError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    if (!is_json) {
      std::cerr << "\nRun the `help` command for usage information\n";
    }

    return EXIT_INVALID_CLI_ARGUMENTS;

  } catch (
      const sourcemeta::core::FileError<sourcemeta::core::GZIPError> &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::HTTPStatusError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_UNEXPECTED_ERROR;
  } catch (const sourcemeta::core::HTTPError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_UNEXPECTED_ERROR;
  } catch (const sourcemeta::core::HTTPSystemBackendError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_NOT_SUPPORTED;

    // Standard library handlers
  } catch (const sourcemeta::core::IOFileNotFoundError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::IOIsADirectoryError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::IOFilePermissionError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::IONotADirectoryError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const sourcemeta::core::IOFileAlreadyExistsError &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const std::filesystem::filesystem_error &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_OTHER_INPUT_ERROR;
  } catch (const std::runtime_error &error) {
    const auto is_json{options.contains("json")};
    print_exception(is_json, error);
    return EXIT_UNEXPECTED_ERROR;
  } catch (const std::exception &error) {
    const auto is_json{options.contains("json")};
    if (is_json) {
      auto error_json{sourcemeta::core::JSON::make_object()};
      error_json.assign("error", sourcemeta::core::JSON{error.what()});
      sourcemeta::core::prettify(error_json, std::cout);
      std::cout << "\n";
    } else {
      std::cerr << "unexpected error: " << error.what()
                << "\nPlease report it at "
                << "https://github.com/sourcemeta/jsonschema\n";
    }

    return EXIT_UNEXPECTED_ERROR;
  }
}

} // namespace sourcemeta::jsonschema

#endif
