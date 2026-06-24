#include <sourcemeta/blaze/foundation_vocabularies.h>

#include <sourcemeta/blaze/foundation_error.h>

#include <cassert>  // assert
#include <optional> // std::optional
#include <sstream>  // std::ostringstream
#include <string>   // std::string
#include <utility>  // std::pair, std::to_underlying
#include <vector>   // std::vector

// X-macro defining all known vocabulary mappings (enum, URI)
// Ordered from most recent/common to oldest for faster short-circuiting
#define SOURCEMETA_VOCABULARIES_X(X)                                           \
  /* 2020-12 vocabularies (most recent/common) */                              \
  X(JSON_Schema_2020_12_Core,                                                  \
    "https://json-schema.org/draft/2020-12/vocab/core")                        \
  X(JSON_Schema_2020_12_Applicator,                                            \
    "https://json-schema.org/draft/2020-12/vocab/applicator")                  \
  X(JSON_Schema_2020_12_Unevaluated,                                           \
    "https://json-schema.org/draft/2020-12/vocab/unevaluated")                 \
  X(JSON_Schema_2020_12_Validation,                                            \
    "https://json-schema.org/draft/2020-12/vocab/validation")                  \
  X(JSON_Schema_2020_12_Meta_Data,                                             \
    "https://json-schema.org/draft/2020-12/vocab/meta-data")                   \
  X(JSON_Schema_2020_12_Format_Annotation,                                     \
    "https://json-schema.org/draft/2020-12/vocab/format-annotation")           \
  X(JSON_Schema_2020_12_Format_Assertion,                                      \
    "https://json-schema.org/draft/2020-12/vocab/format-assertion")            \
  X(JSON_Schema_2020_12_Content,                                               \
    "https://json-schema.org/draft/2020-12/vocab/content")                     \
  /* 2019-09 vocabularies */                                                   \
  X(JSON_Schema_2019_09_Core,                                                  \
    "https://json-schema.org/draft/2019-09/vocab/core")                        \
  X(JSON_Schema_2019_09_Applicator,                                            \
    "https://json-schema.org/draft/2019-09/vocab/applicator")                  \
  X(JSON_Schema_2019_09_Validation,                                            \
    "https://json-schema.org/draft/2019-09/vocab/validation")                  \
  X(JSON_Schema_2019_09_Meta_Data,                                             \
    "https://json-schema.org/draft/2019-09/vocab/meta-data")                   \
  X(JSON_Schema_2019_09_Format,                                                \
    "https://json-schema.org/draft/2019-09/vocab/format")                      \
  X(JSON_Schema_2019_09_Content,                                               \
    "https://json-schema.org/draft/2019-09/vocab/content")                     \
  X(JSON_Schema_2019_09_Hyper_Schema,                                          \
    "https://json-schema.org/draft/2019-09/vocab/hyper-schema")                \
  /* Pre-vocabulary dialects (least common, checked last) */                   \
  X(JSON_Schema_Draft_7, "http://json-schema.org/draft-07/schema#")            \
  X(JSON_Schema_Draft_7_Hyper,                                                 \
    "http://json-schema.org/draft-07/hyper-schema#")                           \
  X(JSON_Schema_Draft_6, "http://json-schema.org/draft-06/schema#")            \
  X(JSON_Schema_Draft_6_Hyper,                                                 \
    "http://json-schema.org/draft-06/hyper-schema#")                           \
  X(JSON_Schema_Draft_4, "http://json-schema.org/draft-04/schema#")            \
  X(JSON_Schema_Draft_4_Hyper,                                                 \
    "http://json-schema.org/draft-04/hyper-schema#")                           \
  X(JSON_Schema_Draft_3, "http://json-schema.org/draft-03/schema#")            \
  X(JSON_Schema_Draft_3_Hyper,                                                 \
    "http://json-schema.org/draft-03/hyper-schema#")                           \
  X(JSON_Schema_Draft_2, "http://json-schema.org/draft-02/schema#")            \
  X(JSON_Schema_Draft_2_Hyper,                                                 \
    "http://json-schema.org/draft-02/hyper-schema#")                           \
  X(JSON_Schema_Draft_1, "http://json-schema.org/draft-01/schema#")            \
  X(JSON_Schema_Draft_1_Hyper,                                                 \
    "http://json-schema.org/draft-01/hyper-schema#")                           \
  X(JSON_Schema_Draft_0, "http://json-schema.org/draft-00/schema#")            \
  X(JSON_Schema_Draft_0_Hyper,                                                 \
    "http://json-schema.org/draft-00/hyper-schema#")                           \
  /* OpenAPI vocabularies */                                                   \
  X(OpenAPI_3_1_Base, "https://spec.openapis.org/oas/3.1/vocab/base")          \
  X(OpenAPI_3_2_Base, "https://spec.openapis.org/oas/3.2/vocab/base")

namespace {
auto uri_to_known_vocabulary(const std::string_view uri)
    -> std::optional<sourcemeta::blaze::Vocabularies::Known> {
  using sourcemeta::blaze::Vocabularies;

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define X_URI_TO_ENUM(enumerator, uri_string)                                  \
  if (uri == (uri_string)) {                                                   \
    return Vocabularies::Known::enumerator;                                    \
  }

  SOURCEMETA_VOCABULARIES_X(X_URI_TO_ENUM)

#undef X_URI_TO_ENUM

  return std::nullopt;
}
} // anonymous namespace

sourcemeta::blaze::Vocabularies::Vocabularies(
    std::initializer_list<std::pair<sourcemeta::core::JSON::String, bool>>
        init) {
  for (const auto &entry : init) {
    this->insert(entry.first, entry.second);
  }
}

sourcemeta::blaze::Vocabularies::Vocabularies(
    std::initializer_list<std::pair<Known, bool>> init) {
  for (const auto &entry : init) {
    this->insert(entry.first, entry.second);
  }
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::Vocabularies::contains(
    const sourcemeta::core::JSON::String &uri) const noexcept -> bool {
  if (this->unknown.has_value()) {
    if (this->unknown->contains(uri)) {
      return true;
    }
  }

  const auto maybe_known{uri_to_known_vocabulary(uri)};
  // As a debug build check: Going through this branch is slow. If it is a
  // known vocabulary, the consumer should be making use of the enum overload
  // of this method
  assert(!maybe_known.has_value());
  if (maybe_known.has_value()) {
    return this->contains(maybe_known.value());
  }

  return false;
}

auto sourcemeta::blaze::Vocabularies::contains(Known vocabulary) const noexcept
    -> bool {
  const auto index = std::to_underlying(vocabulary);
  // Use [] operator instead of test() to avoid exceptions in noexcept function
  return this->required_known[index] || this->optional_known[index];
}

auto sourcemeta::blaze::Vocabularies::contains_any(
    std::initializer_list<Known> vocabularies) const noexcept -> bool {
  for (const auto &vocabulary : vocabularies) {
    if (this->contains(vocabulary)) {
      return true;
    }
  }

  return false;
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::Vocabularies::insert(
    const sourcemeta::core::JSON::String &uri, bool required) noexcept -> void {
  // We NEED to allow official vocabulary string URIs here, as that's how
  // we construct the optimised version!
  const auto maybe_known = uri_to_known_vocabulary(uri);
  if (maybe_known.has_value()) {
    this->insert(maybe_known.value(), required);
  } else {
    if (!this->unknown.has_value()) {
      this->unknown.emplace();
    }
    this->unknown->insert({uri, required});
  }
}

auto sourcemeta::blaze::Vocabularies::insert(Known vocabulary,
                                             bool required) noexcept -> void {
  const auto index = std::to_underlying(vocabulary);
  if (required) {
    this->required_known[index] = true;
    this->optional_known[index] = false;
  } else {
    this->optional_known[index] = true;
    this->required_known[index] = false;
  }
  // Verify invariant: vocabulary cannot be both required and optional
  assert((this->required_known & this->optional_known).none());
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::Vocabularies::get(
    const sourcemeta::core::JSON::String &uri) const noexcept
    -> std::optional<bool> {
  if (this->unknown.has_value()) {
    const auto iterator{this->unknown->find(uri)};
    if (iterator != this->unknown->end()) {
      return iterator->second;
    }
  }

  const auto maybe_known{uri_to_known_vocabulary(uri)};
  // As a debug build check: Going through this branch is slow. If it is a
  // known vocabulary, the consumer should be making use of the enum overload
  // of this method
  assert(!maybe_known.has_value());
  if (maybe_known.has_value()) {
    return this->get(maybe_known.value());
  }

  return std::nullopt;
}

auto sourcemeta::blaze::Vocabularies::get(Known vocabulary) const noexcept
    -> std::optional<bool> {
  const auto index = std::to_underlying(vocabulary);
  // Use [] operator instead of test() to avoid exceptions in noexcept function
  assert(!this->required_known[index] || !this->optional_known[index]);
  if (this->required_known[index]) {
    return true;
  }
  if (this->optional_known[index]) {
    return false;
  }
  return std::nullopt;
}

auto sourcemeta::blaze::Vocabularies::size() const noexcept -> std::size_t {
  return (this->required_known | this->optional_known).count() +
         (this->unknown.has_value() ? this->unknown->size() : 0);
}

auto sourcemeta::blaze::Vocabularies::empty() const noexcept -> bool {
  return this->required_known.none() && this->optional_known.none() &&
         !this->has_unknown();
}

auto sourcemeta::blaze::Vocabularies::has_unknown() const noexcept -> bool {
  return this->unknown.has_value() && !this->unknown->empty();
}

auto sourcemeta::blaze::operator<<(std::ostream &stream,
                                   Vocabularies::Known vocabulary)
    -> std::ostream & {
  switch (vocabulary) {
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define X_ENUM_TO_URI(enumerator, uri_string)                                  \
  case Vocabularies::Known::enumerator:                                        \
    return stream << (uri_string);

    SOURCEMETA_VOCABULARIES_X(X_ENUM_TO_URI)

#undef X_ENUM_TO_URI
  }

  assert(false);
  return stream;
}

auto sourcemeta::blaze::to_string(Vocabularies::Known vocabulary)
    -> std::string_view {
  switch (vocabulary) {
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define X_ENUM_TO_URI(enumerator, uri_string)                                  \
  case Vocabularies::Known::enumerator:                                        \
    return (uri_string);

    SOURCEMETA_VOCABULARIES_X(X_ENUM_TO_URI)

#undef X_ENUM_TO_URI
  }

  assert(false);
  return {};
}

auto sourcemeta::blaze::to_string(const Vocabularies::URI &vocabulary)
    -> std::string_view {
  const auto *known{std::get_if<Vocabularies::Known>(&vocabulary)};
  if (known) {
    return to_string(*known);
  } else {
    return *std::get_if<sourcemeta::core::JSON::String>(&vocabulary);
  }
}

auto sourcemeta::blaze::operator<<(std::ostream &stream,
                                   const Vocabularies::URI &vocabulary)
    -> std::ostream & {
  return stream << to_string(vocabulary);
}

auto sourcemeta::blaze::Vocabularies::throw_if_any_unsupported(
    const std::unordered_set<URI> &supported, const char *message) const
    -> void {
  for (std::size_t index = 0; index < KNOWN_VOCABULARY_COUNT; ++index) {
    if (!this->required_known[index]) {
      continue;
    }

    const auto vocabulary{static_cast<Known>(index)};
    if (supported.contains(vocabulary)) {
      continue;
    }

    // Slow fallback: convert to string URI and check if it was passed as string
    std::ostringstream stream;
    stream << vocabulary;
    const auto &uri{stream.str()};

    if (supported.contains(uri)) {
      // As a debug build check: Going through this branch is slow. If it is a
      // known vocabulary, the consumer should be passing it as an enum class
      assert(false);
      continue;
    }

    throw SchemaVocabularyError(uri, message);
  }

  if (this->unknown.has_value()) {
    for (const auto &[uri, required] : *this->unknown) {
      if (!required || supported.contains(uri)) {
        continue;
      }

      // This case should never be possible, as an invariant of this class.
      // i.e. we should never have an official vocabulary in the unknown map
      assert(!uri_to_known_vocabulary(uri).has_value());

      throw SchemaVocabularyError(uri, message);
    }
  }
}

auto sourcemeta::blaze::Vocabularies::throw_if_any_unknown_required(
    const char *message) const -> void {
  if (this->unknown.has_value()) {
    for (const auto &[uri, required] : this->unknown.value()) {
      if (required) {
        throw SchemaVocabularyError(uri, message);
      }
    }
  }
}
