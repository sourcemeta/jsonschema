#include <sourcemeta/core/openapi.h>

#include <optional>    // std::optional, std::nullopt
#include <string_view> // std::string_view
#include <utility>     // std::unreachable

namespace {
using namespace std::string_view_literals;

constexpr auto HASH_OPENAPI{sourcemeta::core::JSON::Object::hash("openapi"sv)};

// ECMA-262 Section 22.2.2.7 compiles `Atom :: .` by taking every character and,
// when the `dotAll` flag is unset, "Remove from charSet all characters
// corresponding to a code point on the right-hand side of the LineTerminator
// production", which Table 32 lists as U+000A, U+000D, U+2028 and U+2029. JSON
// Schema Validation Section 6.3.3 reads `pattern` "according to the ECMA-262
// regular expression dialect", so the `.` of the meta-schema pattern below
// matches everything except those four
auto contains_line_terminator(const sourcemeta::core::JSON::StringView value)
    -> bool {
  if (value.find_first_of("\x0A\x0D"sv) !=
      sourcemeta::core::JSON::StringView::npos) {
    return true;
  }

  // UTF-8 is self-synchronising, so neither of these sequences can occur
  // within the encoding of any other code point
  return value.find("\xE2\x80\xA8"sv) !=
             sourcemeta::core::JSON::StringView::npos ||
         value.find("\xE2\x80\xA9"sv) !=
             sourcemeta::core::JSON::StringView::npos;
}

// OpenAPI Specification 3.1.1, Section 4.1: "The `major`.`minor` portion of
// the version string (for example `3.1`) SHALL designate the OAS feature set
// [...] The patch version SHOULD NOT be considered by tooling, making no
// distinction between `3.1.0` and `3.1.1` for example". The published
// meta-schemas state the same rule as `^3\.1\.\d+(-.+)?$` and
// `^3\.2\.\d+(-.+)?$`, so a patch component is mandatory, its value carries
// no meaning, and a pre-release suffix of at least one non line terminator
// character is permitted
auto is_openapi_minor(const sourcemeta::core::JSON::StringView version,
                      const sourcemeta::core::JSON::StringView prefix) -> bool {
  if (!version.starts_with(prefix)) {
    return false;
  }

  const auto patch{version.substr(prefix.size())};
  const auto boundary{patch.find_first_not_of("0123456789"sv)};
  if (boundary == 0) {
    return false;
  }

  if (boundary == sourcemeta::core::JSON::StringView::npos) {
    return !patch.empty();
  }

  const auto suffix{patch.substr(boundary)};
  return suffix.starts_with('-') && suffix.size() > 1 &&
         !contains_line_terminator(suffix.substr(1));
}

} // namespace

namespace sourcemeta::core {

auto openapi_version_name(const OpenAPIVersion version) noexcept
    -> JSON::StringView {
  switch (version) {
    // OpenAPI Specification 3.1.1, Section 4.1: "The `major`.`minor` portion
    // of the version string (for example `3.1`) SHALL designate the OAS
    // feature set"
    case OpenAPIVersion::OPENAPI_3_0:
      return "3.0"sv;
    case OpenAPIVersion::OPENAPI_3_1:
      return "3.1"sv;
    case OpenAPIVersion::OPENAPI_3_2:
      return "3.2"sv;
  }

    // Clang settles that no other value arrives, so only GCC and MSVC
    // need to be told that the end of this function is never reached
#if !defined(__clang__)
  std::unreachable();
#endif
}

auto openapi_version_string(const JSON &document)
    -> std::optional<JSON::StringView> {
  if (!document.is_object()) {
    return std::nullopt;
  }

  // OpenAPI Specification 3.1.1, Section 4.8.1: "openapi | string | REQUIRED",
  // so a member of any other type declares no version at all
  const auto *version{document.try_at("openapi", HASH_OPENAPI)};
  if (version == nullptr || !version->is_string()) {
    return std::nullopt;
  }

  return version->to_string();
}

auto openapi_is_document(const JSON &document) -> bool {
  return openapi_version_string(document).has_value();
}

auto openapi_version(const JSON &document) -> std::optional<OpenAPIVersion> {
  // Which feature set a version designates is read from the same bytes that
  // say there is one to read, so the two cannot disagree about what a document
  // declares
  const auto declared{openapi_version_string(document)};
  if (!declared.has_value()) {
    return std::nullopt;
  }

  if (is_openapi_minor(declared.value(), "3.0."sv)) {
    return OpenAPIVersion::OPENAPI_3_0;
  }

  if (is_openapi_minor(declared.value(), "3.1."sv)) {
    return OpenAPIVersion::OPENAPI_3_1;
  }

  if (is_openapi_minor(declared.value(), "3.2."sv)) {
    return OpenAPIVersion::OPENAPI_3_2;
  }

  return std::nullopt;
}

} // namespace sourcemeta::core
