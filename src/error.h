#ifndef SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_
#define SOURCEMETA_JSONSCHEMA_CLI_ERROR_H_

#include <sourcemeta/core/io.h>
#include <sourcemeta/core/options.h>

#include <functional> // std::function

#include "utils.h"

namespace sourcemeta::jsonschema {

inline auto try_catch(const std::function<int()> &callback) noexcept -> int {
  try {
    return callback();
  } catch (const sourcemeta::core::SchemaReferenceError &error) {
    std::cerr << "error: " << error.what() << "\n  " << error.id()
              << "\n    at schema location \"";
    sourcemeta::core::stringify(error.location(), std::cerr);
    std::cerr << "\"\n";
    return EXIT_FAILURE;
  } catch (const sourcemeta::jsonschema::cli::FileError<
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
  } catch (const sourcemeta::jsonschema::cli::FileError<
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
  } catch (const sourcemeta::jsonschema::cli::FileError<
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
