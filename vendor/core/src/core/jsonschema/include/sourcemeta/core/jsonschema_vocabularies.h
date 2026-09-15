#ifndef SOURCEMETA_CORE_JSONSCHEMA_VOCABULARIES_H_
#define SOURCEMETA_CORE_JSONSCHEMA_VOCABULARIES_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <bitset>  // std::bitset
#include <cassert> // assert
#include <cstdint> // std::uint32_t, std::size_t
#include <format> // std::formatter, std::format_context, std::format_parse_context, std::format_to
#include <optional>      // std::optional
#include <ostream>       // std::ostream
#include <sstream>       // std::ostringstream
#include <stdexcept>     // std::out_of_range
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set
#include <utility>       // std::pair
#include <variant>       // std::variant
#include <vector>        // std::vector

namespace sourcemeta::core {

/// @ingroup jsonschema
/// Optimized vocabulary set using bitflags for known vocabularies
/// and a fallback `std::unordered_map` for custom vocabularies.
struct SOURCEMETA_CORE_JSONSCHEMA_EXPORT SchemaVocabularies {
  /// Every vocabulary that this implementation recognises out of the box
  enum class Known : std::uint8_t {
    // Pre-vocabulary dialects (treated as vocabularies)
    /// The Draft 0 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_0 = 0,
    /// The Draft 0 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_0_HYPER = 1,
    /// The Draft 1 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_1 = 2,
    /// The Draft 1 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_1_HYPER = 3,
    /// The Draft 2 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_2 = 4,
    /// The Draft 2 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_2_HYPER = 5,
    /// The Draft 3 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_3 = 6,
    /// The Draft 3 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_3_HYPER = 7,
    /// The Draft 4 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_4 = 8,
    /// The Draft 4 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_4_HYPER = 9,
    /// The Draft 6 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_6 = 10,
    /// The Draft 6 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_6_HYPER = 11,
    /// The Draft 7 dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_7 = 12,
    /// The Draft 7 hyper-schema dialect, which predates vocabularies
    JSON_SCHEMA_DRAFT_7_HYPER = 13,
    // 2019-09 vocabularies
    /// The 2019-09 core vocabulary
    JSON_SCHEMA_2019_09_CORE = 14,
    /// The 2019-09 applicator vocabulary
    JSON_SCHEMA_2019_09_APPLICATOR = 15,
    /// The 2019-09 validation vocabulary
    JSON_SCHEMA_2019_09_VALIDATION = 16,
    /// The 2019-09 meta-data vocabulary
    JSON_SCHEMA_2019_09_META_DATA = 17,
    /// The 2019-09 format vocabulary
    JSON_SCHEMA_2019_09_FORMAT = 18,
    /// The 2019-09 content vocabulary
    JSON_SCHEMA_2019_09_CONTENT = 19,
    /// The 2019-09 hyper-schema vocabulary
    JSON_SCHEMA_2019_09_HYPER_SCHEMA = 20,
    // 2020-12 vocabularies
    /// The 2020-12 core vocabulary
    JSON_SCHEMA_2020_12_CORE = 21,
    /// The 2020-12 applicator vocabulary
    JSON_SCHEMA_2020_12_APPLICATOR = 22,
    /// The 2020-12 unevaluated vocabulary
    JSON_SCHEMA_2020_12_UNEVALUATED = 23,
    /// The 2020-12 validation vocabulary
    JSON_SCHEMA_2020_12_VALIDATION = 24,
    /// The 2020-12 meta-data vocabulary
    JSON_SCHEMA_2020_12_META_DATA = 25,
    /// The 2020-12 format vocabulary, in its annotation form
    JSON_SCHEMA_2020_12_FORMAT_ANNOTATION = 26,
    /// The 2020-12 format vocabulary, in its assertion form
    JSON_SCHEMA_2020_12_FORMAT_ASSERTION = 27,
    /// The 2020-12 content vocabulary
    JSON_SCHEMA_2020_12_CONTENT = 28,
    // OpenAPI
    // https://spec.openapis.org/oas/v3.1.0.html#fixed-fields-19
    /// The OpenAPI 3.1 base vocabulary
    OPENAPI_3_1_BASE = 29,
    // https://spec.openapis.org/oas/v3.2.0.html#base-vocabulary
    /// The OpenAPI 3.2 base vocabulary
    OPENAPI_3_2_BASE = 30,
    // Sourcemeta
    /// The first version of the Sourcemeta extension vocabulary
    SOURCEMETA_EXTENSION_V1 = 31
  };

  // NOTE: Must be kept in sync with the Known enum above
  /// How many vocabularies this implementation recognises out of the box
  static constexpr std::size_t KNOWN_VOCABULARY_COUNT = 32;

  /// A vocabulary URI type that can be either a known vocabulary enum or a
  /// custom string URI
  using URI = std::variant<Known, sourcemeta::core::JSON::String>;

  /// A vocabulary URI that does not own its custom string, for tables that
  /// point at storage that outlives them
  using URIView = std::variant<Known, std::string_view>;

public:
  SchemaVocabularies() = default;
  /// Copy a vocabulary set
  SchemaVocabularies(const SchemaVocabularies &) = default;
  /// Move a vocabulary set
  SchemaVocabularies(SchemaVocabularies &&) noexcept = default;
  auto operator=(const SchemaVocabularies &) -> SchemaVocabularies & = default;
  auto operator=(SchemaVocabularies &&) noexcept
      -> SchemaVocabularies & = default;
  ~SchemaVocabularies() = default;

  /// Construct from initializer list
  SchemaVocabularies(
      std::initializer_list<std::pair<sourcemeta::core::JSON::String, bool>>
          init);

  /// Construct from initializer list using known vocabulary enums
  SchemaVocabularies(std::initializer_list<std::pair<Known, bool>> init);

  /// Check if a vocabulary is enabled
  [[nodiscard]] auto
  contains(const sourcemeta::core::JSON::String &uri) const noexcept -> bool;

  /// Check if a known vocabulary is enabled
  [[nodiscard]] auto contains(Known vocabulary) const noexcept -> bool;

  /// Check if any of the given known vocabularies are enabled
  [[nodiscard]] auto
  contains_any(std::initializer_list<Known> vocabularies) const noexcept
      -> bool;

  /// Insert a vocabulary with its required/optional status
  auto insert(const sourcemeta::core::JSON::String &uri, bool required) noexcept
      -> void;

  /// Insert a known vocabulary with its required/optional status
  auto insert(Known vocabulary, bool required) noexcept -> void;

  /// Get vocabulary status by URI
  [[nodiscard]] auto
  get(const sourcemeta::core::JSON::String &uri) const noexcept
      -> std::optional<bool>;

  /// Get known vocabulary status
  [[nodiscard]] auto get(Known vocabulary) const noexcept
      -> std::optional<bool>;

  /// Get the number of vocabularies (required + optional + custom)
  [[nodiscard]] auto size() const noexcept -> std::size_t;

  /// Check if there are no vocabularies
  [[nodiscard]] auto empty() const noexcept -> bool;

  /// Check if there are any unknown vocabularies
  [[nodiscard]] auto has_unknown() const noexcept -> bool;

  /// Iterate over every vocabulary, along with whether it is required
  template <typename Callback>
  auto for_each(const Callback &callback) const -> void {
    for (std::size_t index = 0; index < KNOWN_VOCABULARY_COUNT; ++index) {
      if (this->required_known_[index]) {
        callback(URI{static_cast<Known>(index)}, true);
      } else if (this->optional_known_[index]) {
        callback(URI{static_cast<Known>(index)}, false);
      }
    }

    if (this->unknown_.has_value()) {
      for (const auto &[uri, required] : this->unknown_.value()) {
        callback(URI{uri}, required);
      }
    }
  }

  /// Throw if the current vocabularies have required ones outside the given
  /// supported set
  auto throw_if_any_unsupported(const std::unordered_set<URI> &supported,
                                const char *message) const -> void;

private:
  // Invariant: required_known and optional_known must be mutually exclusive
  // A vocabulary can be either required (true) OR optional (false), never both
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  std::bitset<KNOWN_VOCABULARY_COUNT> required_known_{};
  std::bitset<KNOWN_VOCABULARY_COUNT> optional_known_{};
  // Lazily initialized only when unknown (non-official) vocabularies are used
  std::optional<std::unordered_map<sourcemeta::core::JSON::String, bool>>
      unknown_{std::nullopt};
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

/// Convert a known vocabulary enum to its URI string
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto
operator<<(std::ostream &stream, SchemaVocabularies::Known vocabulary)
    -> std::ostream &;

/// Convert a vocabulary URI to its string representation
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto
operator<<(std::ostream &stream, const SchemaVocabularies::URI &vocabulary)
    -> std::ostream &;

} // namespace sourcemeta::core

/// @cond
template <> struct std::formatter<sourcemeta::core::SchemaVocabularies::Known> {
  constexpr auto parse(std::format_parse_context &context)
      -> decltype(context.begin()) {
    return context.begin();
  }

  auto format(const sourcemeta::core::SchemaVocabularies::Known value,
              std::format_context &context) const -> decltype(context.out()) {
    std::ostringstream stream;
    stream << value;
    return std::format_to(context.out(), "{}", stream.str());
  }
};

template <> struct std::formatter<sourcemeta::core::SchemaVocabularies::URI> {
  constexpr auto parse(std::format_parse_context &context)
      -> decltype(context.begin()) {
    return context.begin();
  }

  auto format(const sourcemeta::core::SchemaVocabularies::URI &value,
              std::format_context &context) const -> decltype(context.out()) {
    std::ostringstream stream;
    stream << value;
    return std::format_to(context.out(), "{}", stream.str());
  }
};
/// @endcond

#endif
