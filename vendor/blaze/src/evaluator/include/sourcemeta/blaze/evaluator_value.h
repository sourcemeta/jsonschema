#ifndef SOURCEMETA_BLAZE_EVALUATOR_VALUE_H
#define SOURCEMETA_BLAZE_EVALUATOR_VALUE_H

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/regex.h>

#include <cstdint>       // std::uint8_t
#include <optional>      // std::optional
#include <string>        // std::string
#include <tuple>         // std::tuple
#include <unordered_set> // std::unordered_set
#include <utility>       // std::pair
#include <vector>        // std::vector

namespace sourcemeta::blaze {

/// @ingroup evaluator
/// @brief Represents a compiler step empty value
struct ValueNone {
  [[nodiscard]] auto to_json() const -> sourcemeta::core::JSON {
    return sourcemeta::core::JSON{nullptr};
  }

  static auto from_json(const sourcemeta::core::JSON &)
      -> std::optional<ValueNone> {
    return ValueNone{};
  }
};

/// @ingroup evaluator
/// Represents a compiler step JSON value
using ValueJSON = sourcemeta::core::JSON;

/// @ingroup evaluator
/// Represents a set of JSON values
using ValueSet =
    std::unordered_set<sourcemeta::core::JSON,
                       sourcemeta::core::HashJSON<sourcemeta::core::JSON>>;

/// @ingroup evaluator
/// Represents a compiler step string value
using ValueString = sourcemeta::core::JSON::String;

/// @ingroup evaluator
/// Represents a compiler step object property value
using ValueProperty =
    std::pair<ValueString, sourcemeta::core::JSON::Object::hash_type>;

/// @ingroup evaluator
/// Represents a compiler step string values
using ValueStrings = std::vector<ValueString>;

/// @ingroup evaluator
/// Represents a compiler step string set of values
using ValueStringSet = sourcemeta::core::JSONPropertySet;

/// @ingroup evaluator
/// Represents a compiler step JSON types value as a bitmask
using ValueTypes = sourcemeta::core::JSON::TypeSet;

/// @ingroup evaluator
/// Represents a compiler step JSON type value
using ValueType = sourcemeta::core::JSON::Type;

/// @ingroup evaluator
/// Represents a compiler step ECMA regular expression value. We store both the
/// original string and the regular expression as standard regular expressions
/// do not keep a copy of their original value (which we need for serialization
/// purposes)
// NOLINTNEXTLINE(bugprone-exception-escape)
struct ValueRegex {
  using second_type = ValueString;
  using first_type = sourcemeta::core::Regex;
  first_type first;
  second_type second;

  [[nodiscard]] auto to_json() const -> sourcemeta::core::JSON {
    return sourcemeta::core::to_json(this->second);
  }

  static auto from_json(const sourcemeta::core::JSON &value)
      -> std::optional<ValueRegex> {
    if (!value.is_string()) {
      return std::nullopt;
    }

    auto string{value.to_string()};
    auto regex{sourcemeta::core::to_regex(string)};
    if (!regex.has_value()) {
      return std::nullopt;
    }

    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    return ValueRegex{std::move(regex).value(), std::move(string)};
  }
};

/// @ingroup evaluator
/// Represents a compiler step JSON unsigned integer value
using ValueUnsignedInteger = std::size_t;

/// @ingroup evaluator
/// Represents a compiler step range value. The boolean option
/// modifies whether the range is considered exhaustively or
/// if the evaluator is allowed to break early
using ValueRange = std::tuple<std::size_t, std::optional<std::size_t>, bool>;

/// @ingroup evaluator
/// Represents a compiler step boolean value
using ValueBoolean = bool;

/// @ingroup evaluator
/// Represents a compiler step string logical type
enum class ValueStringType : std::uint8_t {
  URI,
  URIReference,
  URITemplate,
  IRI,
  IRIReference,
  Email,
  IDNEmail,
  IPv4,
  IPv6,
  Hostname,
  IDNHostname,
  DateTime,
  Date,
  Time,
  PartialTime,
  Duration,
  JSONPointer,
  RelativeJSONPointer,
  UUID,
  Regex,
  Color,
  Base64
};

/// @ingroup evaluator
/// Represents an compiler step that maps strings to strings
using ValueStringMap = sourcemeta::core::JSONObject<
    ValueString, ValueStrings, sourcemeta::core::PropertyHashJSON<ValueString>>;

/// @ingroup evaluator
/// Represents a compiler step value that consist of object property filters
/// (strings, prefixes, regexes)
using ValuePropertyFilter =
    std::tuple<ValueStringSet, ValueStrings, std::vector<ValueRegex>>;

/// @ingroup evaluator
/// Represents a compiler step value that consists of two indexes
using ValueIndexPair = std::pair<std::size_t, std::size_t>;

/// @ingroup evaluator
/// Represents a compiler step value that consists of a pointer
using ValuePointer = sourcemeta::core::Pointer;

/// @ingroup evaluator
/// Represents the precomputed hash of a property name, the index of the child
/// instruction that applies to it, and the name itself. The hash comes first
/// as it is what a lookup compares against
using ValueNamedIndex = std::tuple<sourcemeta::core::JSON::Object::hash_type,
                                   ValueUnsignedInteger, ValueString>;

/// @ingroup evaluator
/// Represents a compiler step string to index map. The entries are sorted by
/// the size of the name, and the table of contents gives, for every name size,
/// the one-based inclusive range of entries that have that size, so that a
/// lookup only ever compares the names whose size can match
using ValueNamedIndexes =
    std::pair<std::vector<ValueNamedIndex>, std::vector<ValueIndexPair>>;

/// @ingroup evaluator
/// Represents a compiler step types properties value
using ValueTypedProperties = std::pair<ValueType, ValueStringSet>;

/// @ingroup evaluator
/// Represents a compiler step types property hashes value
using ValueStringHashes =
    std::pair<std::vector<std::pair<ValueStringSet::hash_type, ValueString>>,
              std::vector<ValueIndexPair>>;

/// @ingroup evaluator
/// Represents a compiler step types property hashes value
using ValueTypedHashes = std::pair<ValueType, ValueStringHashes>;

/// @ingroup evaluator
/// Represents integer bounds with minimum and maximum
using ValueIntegerBounds = std::pair<std::int64_t, std::int64_t>;

/// @ingroup evaluator
/// Represents integer bounds combined with array size range
using ValueIntegerBoundsWithSize = std::pair<ValueIntegerBounds, ValueRange>;

/// @ingroup evaluator
/// Represents a set of types combined with an array size range
using ValueTypesWithSize = std::pair<ValueTypes, ValueRange>;

/// @ingroup evaluator
/// Represents a list of object property entries: (name, hash, required)
using ValueObjectProperties = std::vector<
    std::tuple<ValueString, sourcemeta::core::JSON::Object::hash_type, bool>>;

/// @ingroup evaluator
/// Represents a compiler step number logical type
enum class ValueNumberType : std::uint8_t { Float, Double };

/// @ingroup evaluator
using Value =
    std::variant<ValueNone, ValueJSON, ValueSet, ValueString, ValueProperty,
                 ValueStrings, ValueStringSet, ValueTypes, ValueType,
                 ValueRegex, ValueUnsignedInteger, ValueRange, ValueBoolean,
                 ValueNamedIndexes, ValueStringType, ValueStringMap,
                 ValuePropertyFilter, ValueIndexPair, ValuePointer,
                 ValueTypedProperties, ValueStringHashes, ValueTypedHashes,
                 ValueIntegerBounds, ValueIntegerBoundsWithSize,
                 ValueObjectProperties, ValueNumberType, ValueTypesWithSize>;

} // namespace sourcemeta::blaze

#endif
