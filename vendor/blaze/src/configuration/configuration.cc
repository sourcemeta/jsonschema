#include <sourcemeta/blaze/configuration.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/jsonpointer.h>

#include <algorithm>    // std::ranges::any_of
#include <cassert>      // assert
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::error_code
#include <utility>      // std::move

namespace {

// The `.json` extension is not a URI concern, so this single alternative is
// tried by hand, and always after the identifier itself, so that a mapping
// that holds both spellings is unambiguous. Only a path can carry the
// extension, so a query or a fragment stays where it is
auto resolve_alternative(const std::string_view identifier)
    -> sourcemeta::core::JSON::String {
  constexpr std::string_view JSON_EXTENSION{".json"};
  const auto suffix{identifier.find_first_of("?#")};
  const auto path_size{suffix == std::string_view::npos ? identifier.size()
                                                        : suffix};

  sourcemeta::core::JSON::String result{identifier};
  if (identifier.substr(0, path_size).ends_with(JSON_EXTENSION)) {
    result.erase(path_size - JSON_EXTENSION.size(), JSON_EXTENSION.size());
  } else {
    result.insert(path_size, JSON_EXTENSION);
  }

  return result;
}

} // namespace

namespace sourcemeta::blaze {

auto Configuration::add_dependency(const sourcemeta::core::URI &uri,
                                   const std::filesystem::path &path) -> void {
  assert(path.is_absolute());
  const auto canonical_uri{
      sourcemeta::core::URI::canonicalize(uri.recompose())};

  if (this->dependencies.contains(canonical_uri)) [[unlikely]] {
    throw ConfigurationParseError(
        "The dependency already exists",
        sourcemeta::core::Pointer({"dependencies", canonical_uri}));
  }

  for (const auto &existing : this->dependencies) {
    if (existing.second == path) [[unlikely]] {
      throw ConfigurationParseError(
          "Multiple dependencies cannot point to the same path",
          sourcemeta::core::Pointer({"dependencies", existing.first}));
    }
  }

  this->dependencies.emplace(canonical_uri, path);
}

auto Configuration::add_resolve(
    const sourcemeta::core::JSON::String &identifier,
    const sourcemeta::core::JSON::String &target) -> void {
  // Keys are canonicalized here, once, rather than on every lookup, which
  // would make resolution linear in the size of the mapping for each reference
  auto canonical_identifier{
      sourcemeta::core::URI::try_canonicalize(identifier)};
  if (!canonical_identifier.has_value()) [[unlikely]] {
    throw ConfigurationParseError(
        "The keys in the resolve object must represent valid URIs",
        sourcemeta::core::Pointer({"resolve", identifier}));
  }

  auto canonical_target{sourcemeta::core::URI::try_canonicalize(target)};
  if (!canonical_target.has_value()) [[unlikely]] {
    throw ConfigurationParseError(
        "The values in the resolve object must represent valid URIs",
        sourcemeta::core::Pointer({"resolve", identifier}));
  }

  const auto inserted{
      this->resolve.emplace(std::move(canonical_identifier).value(),
                            std::move(canonical_target).value())};
  if (!inserted.second) [[unlikely]] {
    throw ConfigurationParseError(
        "Two keys in the resolve object denote the same URI",
        sourcemeta::core::Pointer({"resolve", identifier}));
  }
}

auto Configuration::find(const std::filesystem::path &path)
    -> std::optional<std::filesystem::path> {
  // Note we use non-throwing overloads of filesystem functions to gracefully
  // handle I/O errors on FUSE and other unusual filesystems
  std::filesystem::path canonical;
  try {
    canonical = sourcemeta::core::weakly_canonical(path);
  } catch (const std::filesystem::filesystem_error &) {
    return std::nullopt;
  }

  assert(canonical.is_absolute());
  std::error_code error;
  const auto is_directory = std::filesystem::is_directory(canonical, error);
  auto current = !error && !is_directory ? canonical.parent_path() : canonical;

  while (!current.empty()) {
    auto candidate = current / "jsonschema.json";
    if (std::filesystem::exists(candidate, error) &&
        std::filesystem::is_regular_file(candidate, error)) {
      return candidate;
    }

    auto parent = current.parent_path();
    if (parent == current) {
      break;
    }
    current = parent;
  }

  return std::nullopt;
}

auto Configuration::applies_to(const std::filesystem::path &path) const
    -> bool {
  if (this->extension.empty()) {
    return true;
  }

  const std::string filename{path.filename().string()};
  return std::ranges::any_of(this->extension,
                             [&path, &filename](const auto &suffix) -> auto {
                               if (suffix.empty()) {
                                 return path.extension().empty();
                               }

                               return filename.ends_with(suffix);
                             });
}

auto Configuration::find_resolve(
    const sourcemeta::core::JSON::String &identifier) const
    -> const sourcemeta::core::JSON::String * {
  // Most configurations declare no mapping at all, and every reference of
  // every schema comes through here, so that case does no work
  if (this->resolve.empty()) {
    return nullptr;
  }

  // Keys are stored canonicalized, so an identifier spelled the way its key is
  // matches without parsing a URI at all
  auto match{this->resolve.find(identifier)};
  if (match != this->resolve.cend()) {
    return &match->second;
  }

  match = this->resolve.find(resolve_alternative(identifier));
  if (match != this->resolve.cend()) {
    return &match->second;
  }

  // Comparing canonical URIs is what lets a key match however the identifier
  // was spelled. An identifier that no URI can express falls through as
  // unmapped rather than refusing the lookup outright
  const auto canonical{sourcemeta::core::URI::try_canonicalize(identifier)};
  if (!canonical.has_value() || canonical.value() == identifier) {
    return nullptr;
  }

  match = this->resolve.find(canonical.value());
  if (match != this->resolve.cend()) {
    return &match->second;
  }

  match = this->resolve.find(resolve_alternative(canonical.value()));
  return match == this->resolve.cend() ? nullptr : &match->second;
}

} // namespace sourcemeta::blaze
