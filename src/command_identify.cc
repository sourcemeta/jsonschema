#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonschema.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream> // std::cout

#include "command.h"
#include "utils.h"

static auto
find_relative_to(const std::map<std::string, std::vector<std::string>> &options)
    -> std::optional<std::string> {
  if (options.contains("relative-to") && !options.at("relative-to").empty()) {
    return options.at("relative-to").front();
  } else if (options.contains("t") && !options.at("t").empty()) {
    return options.at("t").front();
  }

  return std::nullopt;
}

auto sourcemeta::jsonschema::cli::identify(
    const std::span<const std::string> &arguments) -> int {
  const auto options{parse_options(arguments, {})};
  if (options.at("").size() < 1) {
    std::cerr
        << "error: This command expects a path to a schema. For example:\n\n"
        << "  jsonschema identify path/to/schema.json\n";
    return EXIT_FAILURE;
  }

  const sourcemeta::jsontoolkit::JSON schema{
      sourcemeta::jsontoolkit::from_file(options.at("").front())};

  // Just to print a nice warning
  try {
    const auto base_dialect{
        sourcemeta::jsontoolkit::base_dialect(
            schema, sourcemeta::jsontoolkit::official_resolver)
            .get()};
    if (!base_dialect.has_value()) {
      std::cerr << "warning: Cannot determine the base dialect of the schema, "
                   "but will attempt to guess\n";
    }
  } catch (const sourcemeta::jsontoolkit::SchemaResolutionError &) {
    std::cerr << "warning: Cannot determine the base dialect of the schema, "
                 "but will attempt to guess\n";
  }

  std::optional<std::string> identifier;

  try {
    identifier = sourcemeta::jsontoolkit::identify(
                     schema, sourcemeta::jsontoolkit::official_resolver,
                     sourcemeta::jsontoolkit::IdentificationStrategy::Loose)
                     .get();
  } catch (const sourcemeta::jsontoolkit::SchemaError &error) {
    std::cerr
        << "error: " << error.what() << "\n  "
        << std::filesystem::weakly_canonical(options.at("").front()).string()
        << "\n";
    return EXIT_FAILURE;
  }

  if (!identifier.has_value()) {
    std::cerr
        << "error: Could not determine schema identifier\n  "
        << std::filesystem::weakly_canonical(options.at("").front()).string()
        << "\n";
    return EXIT_FAILURE;
  }

  std::string result;

  try {
    result = sourcemeta::jsontoolkit::URI{identifier.value()}
                 .canonicalize()
                 .recompose();
  } catch (const sourcemeta::jsontoolkit::URIParseError &error) {
    std::cerr
        << "error: Invalid schema identifier URI at column " << error.column()
        << "\n  "
        << std::filesystem::weakly_canonical(options.at("").front()).string()
        << "\n";
    return EXIT_FAILURE;
  }

  const auto relative_to{find_relative_to(options)};
  if (relative_to.has_value()) {
    log_verbose(options) << "Resolving identifier against: "
                         << relative_to.value() << "\n";
    std::string base;

    try {
      base = sourcemeta::jsontoolkit::URI{relative_to.value()}
                 .canonicalize()
                 .recompose();
    } catch (const sourcemeta::jsontoolkit::URIParseError &error) {
      std::cerr << "error: Invalid base URI at column " << error.column()
                << "\n  " << relative_to.value() << "\n";
      return EXIT_FAILURE;
    }

    sourcemeta::jsontoolkit::URI base_uri{base};
    base_uri.canonicalize();
    sourcemeta::jsontoolkit::URI uri{result};
    uri.canonicalize();
    uri.relative_to(base_uri);

    if (!uri.is_absolute()) {
      if (uri.recompose().empty()) {
        std::cerr << "error: the base URI cannot be equal to the schema "
                     "identifier\n  "
                  << std::filesystem::weakly_canonical(options.at("").front())
                         .string()
                  << "\n";
        return EXIT_FAILURE;
      }

      std::cout << uri.recompose() << "\n";
    } else {
      std::cerr
          << "error: the schema identifier " << result
          << " is not resolvable from " << base << "\n  "
          << std::filesystem::weakly_canonical(options.at("").front()).string()
          << "\n";
      return EXIT_FAILURE;
    }
  } else {
    std::cout << result << "\n";
  }

  return EXIT_SUCCESS;
}
