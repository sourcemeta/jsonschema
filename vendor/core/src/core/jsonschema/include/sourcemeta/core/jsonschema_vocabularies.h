#ifndef SOURCEMETA_CORE_JSONSCHEMA_VOCABULARIES_H_
#define SOURCEMETA_CORE_JSONSCHEMA_VOCABULARIES_H_

#ifndef SOURCEMETA_CORE_JSONSCHEMA_EXPORT
#include <sourcemeta/core/jsonschema_export.h>
#endif

#include <sourcemeta/core/json.h>

#include <bitset>        // std::bitset
#include <cassert>       // assert
#include <cstdint>       // std::uint32_t, std::size_t
#include <optional>      // std::optional
#include <ostream>       // std::ostream
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
struct SOURCEMETA_CORE_JSONSCHEMA_EXPORT Vocabularies {
  enum class Known : std::uint8_t {
    // Pre-vocabulary dialects (treated as vocabularies)
    JSON_Schema_Draft_0 = 0,
    JSON_Schema_Draft_0_Hyper = 1,
    JSON_Schema_Draft_1 = 2,
    JSON_Schema_Draft_1_Hyper = 3,
    JSON_Schema_Draft_2 = 4,
    JSON_Schema_Draft_2_Hyper = 5,
    JSON_Schema_Draft_3 = 6,
    JSON_Schema_Draft_3_Hyper = 7,
    JSON_Schema_Draft_4 = 8,
    JSON_Schema_Draft_4_Hyper = 9,
    JSON_Schema_Draft_6 = 10,
    JSON_Schema_Draft_6_Hyper = 11,
    JSON_Schema_Draft_7 = 12,
    JSON_Schema_Draft_7_Hyper = 13,
    // 2019-09 vocabularies
    JSON_Schema_2019_09_Core = 14,
    JSON_Schema_2019_09_Applicator = 15,
    JSON_Schema_2019_09_Validation = 16,
    JSON_Schema_2019_09_Meta_Data = 17,
    JSON_Schema_2019_09_Format = 18,
    JSON_Schema_2019_09_Content = 19,
    JSON_Schema_2019_09_Hyper_Schema = 20,
    // 2020-12 vocabularies
    JSON_Schema_2020_12_Core = 21,
    JSON_Schema_2020_12_Applicator = 22,
    JSON_Schema_2020_12_Unevaluated = 23,
    JSON_Schema_2020_12_Validation = 24,
    JSON_Schema_2020_12_Meta_Data = 25,
    JSON_Schema_2020_12_Format_Annotation = 26,
    JSON_Schema_2020_12_Format_Assertion = 27,
    JSON_Schema_2020_12_Content = 28,
    // OpenAPI
    // https://spec.openapis.org/oas/v3.1.0.html#fixed-fields-19
    OpenAPI_3_1_Base = 29,
    // https://spec.openapis.org/oas/v3.2.0.html#base-vocabulary
    OpenAPI_3_2_Base = 30
  };

  // NOTE: Must be kept in sync with the Known enum above
  static constexpr std::size_t KNOWN_VOCABULARY_COUNT = 31;

  /// A vocabulary URI type that can be either a known vocabulary enum or a
  /// custom string URI
  using URI = std::variant<Known, JSON::String>;

public:
  Vocabularies() = default;
  Vocabularies(const Vocabularies &) = default;
  Vocabularies(Vocabularies &&) noexcept = default;
  auto operator=(const Vocabularies &) -> Vocabularies & = default;
  auto operator=(Vocabularies &&) noexcept -> Vocabularies & = default;
  ~Vocabularies() = default;

  /// Construct from initializer list
  Vocabularies(std::initializer_list<std::pair<JSON::String, bool>> init);

  /// Construct from initializer list using known vocabulary enums
  Vocabularies(std::initializer_list<std::pair<Known, bool>> init);

  /// Check if a vocabulary is enabled
  [[nodiscard]] auto contains(const JSON::String &uri) const noexcept -> bool;

  /// Check if a known vocabulary is enabled
  [[nodiscard]] auto contains(Known vocabulary) const noexcept -> bool;

  /// Check if any of the given known vocabularies are enabled
  [[nodiscard]] auto
  contains_any(std::initializer_list<Known> vocabularies) const noexcept
      -> bool;

  /// Insert a vocabulary with its required/optional status
  auto insert(const JSON::String &uri, bool required) noexcept -> void;

  /// Insert a known vocabulary with its required/optional status
  auto insert(Known vocabulary, bool required) noexcept -> void;

  /// Get vocabulary status by URI
  [[nodiscard]] auto get(const JSON::String &uri) const noexcept
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

  /// Throw if the current vocabularies have required ones outside the given
  /// supported set
  auto throw_if_any_unsupported(const std::unordered_set<URI> &supported,
                                const char *message) const -> void;

  /// Throw if any unknown vocabulary is required
  auto throw_if_any_unknown_required(const char *message) const -> void;

private:
  // Invariant: required_known and optional_known must be mutually exclusive
  // A vocabulary can be either required (true) OR optional (false), never both
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  std::bitset<KNOWN_VOCABULARY_COUNT> required_known{};
  std::bitset<KNOWN_VOCABULARY_COUNT> optional_known{};
  // Lazily initialized only when unknown (non-official) vocabularies are used
  std::optional<std::unordered_map<JSON::String, bool>> unknown{std::nullopt};
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

/// Convert a known vocabulary enum to its URI string
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto
operator<<(std::ostream &stream, Vocabularies::Known vocabulary)
    -> std::ostream &;

/// Convert a vocabulary URI to its string representation
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto
operator<<(std::ostream &stream, const Vocabularies::URI &vocabulary)
    -> std::ostream &;

/// Stringify a known vocabulary to a string
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto to_string(Vocabularies::Known vocabulary)
    -> std::string_view;

/// Stringify a vocabulary URI to a string
SOURCEMETA_CORE_JSONSCHEMA_EXPORT auto
to_string(const Vocabularies::URI &vocabulary) -> std::string_view;

} // namespace sourcemeta::core

#endif
