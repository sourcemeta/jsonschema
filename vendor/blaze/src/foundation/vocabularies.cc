#include <sourcemeta/blaze/foundation_vocabularies.h>

#include "helpers.h"

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
  X(JSON_SCHEMA_2020_12_CORE,                                                  \
    "https://json-schema.org/draft/2020-12/vocab/core")                        \
  X(JSON_SCHEMA_2020_12_APPLICATOR,                                            \
    "https://json-schema.org/draft/2020-12/vocab/applicator")                  \
  X(JSON_SCHEMA_2020_12_UNEVALUATED,                                           \
    "https://json-schema.org/draft/2020-12/vocab/unevaluated")                 \
  X(JSON_SCHEMA_2020_12_VALIDATION,                                            \
    "https://json-schema.org/draft/2020-12/vocab/validation")                  \
  X(JSON_SCHEMA_2020_12_META_DATA,                                             \
    "https://json-schema.org/draft/2020-12/vocab/meta-data")                   \
  X(JSON_SCHEMA_2020_12_FORMAT_ANNOTATION,                                     \
    "https://json-schema.org/draft/2020-12/vocab/format-annotation")           \
  X(JSON_SCHEMA_2020_12_FORMAT_ASSERTION,                                      \
    "https://json-schema.org/draft/2020-12/vocab/format-assertion")            \
  X(JSON_SCHEMA_2020_12_CONTENT,                                               \
    "https://json-schema.org/draft/2020-12/vocab/content")                     \
  /* 2019-09 vocabularies */                                                   \
  X(JSON_SCHEMA_2019_09_CORE,                                                  \
    "https://json-schema.org/draft/2019-09/vocab/core")                        \
  X(JSON_SCHEMA_2019_09_APPLICATOR,                                            \
    "https://json-schema.org/draft/2019-09/vocab/applicator")                  \
  X(JSON_SCHEMA_2019_09_VALIDATION,                                            \
    "https://json-schema.org/draft/2019-09/vocab/validation")                  \
  X(JSON_SCHEMA_2019_09_META_DATA,                                             \
    "https://json-schema.org/draft/2019-09/vocab/meta-data")                   \
  X(JSON_SCHEMA_2019_09_FORMAT,                                                \
    "https://json-schema.org/draft/2019-09/vocab/format")                      \
  X(JSON_SCHEMA_2019_09_CONTENT,                                               \
    "https://json-schema.org/draft/2019-09/vocab/content")                     \
  X(JSON_SCHEMA_2019_09_HYPER_SCHEMA,                                          \
    "https://json-schema.org/draft/2019-09/vocab/hyper-schema")                \
  /* Pre-vocabulary dialects (least common, checked last) */                   \
  X(JSON_SCHEMA_DRAFT_7, "http://json-schema.org/draft-07/schema#")            \
  X(JSON_SCHEMA_DRAFT_7_HYPER,                                                 \
    "http://json-schema.org/draft-07/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_6, "http://json-schema.org/draft-06/schema#")            \
  X(JSON_SCHEMA_DRAFT_6_HYPER,                                                 \
    "http://json-schema.org/draft-06/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_4, "http://json-schema.org/draft-04/schema#")            \
  X(JSON_SCHEMA_DRAFT_4_HYPER,                                                 \
    "http://json-schema.org/draft-04/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_3, "http://json-schema.org/draft-03/schema#")            \
  X(JSON_SCHEMA_DRAFT_3_HYPER,                                                 \
    "http://json-schema.org/draft-03/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_2, "http://json-schema.org/draft-02/schema#")            \
  X(JSON_SCHEMA_DRAFT_2_HYPER,                                                 \
    "http://json-schema.org/draft-02/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_1, "http://json-schema.org/draft-01/schema#")            \
  X(JSON_SCHEMA_DRAFT_1_HYPER,                                                 \
    "http://json-schema.org/draft-01/hyper-schema#")                           \
  X(JSON_SCHEMA_DRAFT_0, "http://json-schema.org/draft-00/schema#")            \
  X(JSON_SCHEMA_DRAFT_0_HYPER,                                                 \
    "http://json-schema.org/draft-00/hyper-schema#")                           \
  /* OpenAPI vocabularies */                                                   \
  X(OPENAPI_3_1_BASE, "https://spec.openapis.org/oas/3.1/vocab/base")          \
  X(OPENAPI_3_2_BASE, "https://spec.openapis.org/oas/3.2/vocab/base")          \
  /* Sourcemeta vocabularies */                                                \
  X(SOURCEMETA_EXTENSION_V1, "tag:sourcemeta.com,2026:extension/v1")

namespace {
auto uri_to_known_vocabulary(const std::string_view uri)
    -> std::optional<sourcemeta::blaze::SchemaVocabularies::Known> {
  using sourcemeta::blaze::SchemaVocabularies;

// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define X_URI_TO_ENUM(enumerator, uri_string)                                  \
  if (uri == (uri_string)) {                                                   \
    return SchemaVocabularies::Known::enumerator;                              \
  }

  SOURCEMETA_VOCABULARIES_X(X_URI_TO_ENUM)

#undef X_URI_TO_ENUM

  return std::nullopt;
}
} // anonymous namespace

sourcemeta::blaze::SchemaVocabularies::SchemaVocabularies(
    std::initializer_list<std::pair<sourcemeta::core::JSON::String, bool>>
        init) {
  for (const auto &entry : init) {
    this->insert(entry.first, entry.second);
  }
}

sourcemeta::blaze::SchemaVocabularies::SchemaVocabularies(
    std::initializer_list<std::pair<Known, bool>> init) {
  for (const auto &entry : init) {
    this->insert(entry.first, entry.second);
  }
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::SchemaVocabularies::contains(
    const sourcemeta::core::JSON::String &uri) const noexcept -> bool {
  if (this->unknown_.has_value()) {
    if (this->unknown_->contains(uri)) {
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

auto sourcemeta::blaze::SchemaVocabularies::contains(
    Known vocabulary) const noexcept -> bool {
  const auto index = std::to_underlying(vocabulary);
  // Use [] operator instead of test() to avoid exceptions in noexcept function
  return this->required_known_[index] || this->optional_known_[index];
}

auto sourcemeta::blaze::SchemaVocabularies::contains_any(
    std::initializer_list<Known> vocabularies) const noexcept -> bool {
  for (const auto &vocabulary : vocabularies) {
    if (this->contains(vocabulary)) {
      return true;
    }
  }

  return false;
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::SchemaVocabularies::insert(
    const sourcemeta::core::JSON::String &uri, bool required) noexcept -> void {
  // We NEED to allow official vocabulary string URIs here, as that's how
  // we construct the optimised version!
  const auto maybe_known = uri_to_known_vocabulary(uri);
  if (maybe_known.has_value()) {
    this->insert(maybe_known.value(), required);
  } else {
    if (!this->unknown_.has_value()) {
      this->unknown_.emplace();
    }
    this->unknown_->insert({uri, required});
  }
}

auto sourcemeta::blaze::SchemaVocabularies::insert(Known vocabulary,
                                                   bool required) noexcept
    -> void {
  const auto index = std::to_underlying(vocabulary);
  if (required) {
    this->required_known_[index] = true;
    this->optional_known_[index] = false;
  } else {
    this->optional_known_[index] = true;
    this->required_known_[index] = false;
  }
  // Verify invariant: vocabulary cannot be both required and optional
  assert((this->required_known_ & this->optional_known_).none());
}

// NOLINTNEXTLINE(bugprone-exception-escape)
auto sourcemeta::blaze::SchemaVocabularies::get(
    const sourcemeta::core::JSON::String &uri) const noexcept
    -> std::optional<bool> {
  if (this->unknown_.has_value()) {
    const auto iterator{this->unknown_->find(uri)};
    if (iterator != this->unknown_->end()) {
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

auto sourcemeta::blaze::SchemaVocabularies::get(Known vocabulary) const noexcept
    -> std::optional<bool> {
  const auto index = std::to_underlying(vocabulary);
  // Use [] operator instead of test() to avoid exceptions in noexcept function
  assert(!this->required_known_[index] || !this->optional_known_[index]);
  if (this->required_known_[index]) {
    return true;
  }
  if (this->optional_known_[index]) {
    return false;
  }
  return std::nullopt;
}

auto sourcemeta::blaze::SchemaVocabularies::size() const noexcept
    -> std::size_t {
  return (this->required_known_ | this->optional_known_).count() +
         (this->unknown_.has_value() ? this->unknown_->size() : 0);
}

auto sourcemeta::blaze::SchemaVocabularies::empty() const noexcept -> bool {
  return this->required_known_.none() && this->optional_known_.none() &&
         !this->has_unknown();
}

auto sourcemeta::blaze::SchemaVocabularies::has_unknown() const noexcept
    -> bool {
  return this->unknown_.has_value() && !this->unknown_->empty();
}

auto sourcemeta::blaze::operator<<(std::ostream &stream,
                                   SchemaVocabularies::Known vocabulary)
    -> std::ostream & {
  return stream << vocabulary_uri(vocabulary);
}

auto sourcemeta::blaze::vocabulary_uri(SchemaVocabularies::Known vocabulary)
    -> std::string_view {
  switch (vocabulary) {
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define X_ENUM_TO_URI(enumerator, uri_string)                                  \
  case SchemaVocabularies::Known::enumerator:                                  \
    return (uri_string);

    SOURCEMETA_VOCABULARIES_X(X_ENUM_TO_URI)

#undef X_ENUM_TO_URI
  }

  assert(false);
  return {};
}

auto sourcemeta::blaze::vocabulary_uri(
    const SchemaVocabularies::URI &vocabulary) -> std::string_view {
  const auto *known{std::get_if<SchemaVocabularies::Known>(&vocabulary)};
  if (known != nullptr) {
    return vocabulary_uri(*known);
  }
  return *std::get_if<sourcemeta::core::JSON::String>(&vocabulary);
}

auto sourcemeta::blaze::operator<<(std::ostream &stream,
                                   const SchemaVocabularies::URI &vocabulary)
    -> std::ostream & {
  return stream << vocabulary_uri(vocabulary);
}

auto sourcemeta::blaze::SchemaVocabularies::throw_if_any_unsupported(
    const std::unordered_set<URI> &supported, const char *message) const
    -> void {
  for (std::size_t index = 0; index < KNOWN_VOCABULARY_COUNT; ++index) {
    if (!this->required_known_[index]) {
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

  if (this->unknown_.has_value()) {
    for (const auto &[uri, required] : *this->unknown_) {
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
