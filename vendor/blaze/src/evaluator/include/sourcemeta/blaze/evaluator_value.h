#ifndef SOURCEMETA_BLAZE_EVALUATOR_VALUE_H
#define SOURCEMETA_BLAZE_EVALUATOR_VALUE_H

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonpointer.h>
#include <sourcemeta/jsontoolkit/regex.h>
#include <sourcemeta/jsontoolkit/uri.h>

#include <sourcemeta/blaze/evaluator_string_set.h>

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
struct ValueNone {};

/// @ingroup evaluator
/// Represents a compiler step JSON value
using ValueJSON = sourcemeta::jsontoolkit::JSON;

/// @ingroup evaluator
/// Represents a set of JSON values
using ValueSet = std::unordered_set<sourcemeta::jsontoolkit::JSON,
                                    sourcemeta::jsontoolkit::Hash>;

/// @ingroup evaluator
/// Represents a compiler step string value
using ValueString = sourcemeta::jsontoolkit::JSON::String;

/// @ingroup evaluator
/// Represents a compiler step object property value
using ValueProperty =
    std::pair<ValueString,
              sourcemeta::jsontoolkit::JSON::Object::Container::hash_type>;

/// @ingroup evaluator
/// Represents a compiler step string values
using ValueStrings = std::vector<ValueString>;

/// @ingroup evaluator
/// Represents a compiler step string set of values
using ValueStringSet = StringSet;

/// @ingroup evaluator
/// Represents a compiler step JSON types value
using ValueTypes = std::vector<sourcemeta::jsontoolkit::JSON::Type>;

/// @ingroup evaluator
/// Represents a compiler step JSON type value
using ValueType = sourcemeta::jsontoolkit::JSON::Type;

/// @ingroup evaluator
/// Represents a compiler step ECMA regular expression value. We store both the
/// original string and the regular expression as standard regular expressions
/// do not keep a copy of their original value (which we need for serialization
/// purposes)
using ValueRegex = std::pair<sourcemeta::jsontoolkit::Regex, std::string>;

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
/// Represents a compiler step string to index map
using ValueNamedIndexes =
    sourcemeta::jsontoolkit::FlatMap<ValueString, ValueUnsignedInteger,
                                     sourcemeta::jsontoolkit::Hash>;

/// @ingroup evaluator
/// Represents a compiler step string logical type
enum class ValueStringType : std::uint8_t { URI };

/// @ingroup evaluator
/// Represents an compiler step that maps strings to strings
using ValueStringMap =
    sourcemeta::jsontoolkit::FlatMap<ValueString, ValueStrings,
                                     sourcemeta::jsontoolkit::Hash>;

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
using ValuePointer = sourcemeta::jsontoolkit::Pointer;

/// @ingroup evaluator
/// Represents a compiler step types properties value
using ValueTypedProperties = std::pair<ValueType, ValueStringSet>;

/// @ingroup evaluator
/// Represents a compiler step types property hashes value
using ValueTypedHashes =
    std::pair<ValueType, std::vector<ValueStringSet::hash_type>>;

/// @ingroup evaluator
using Value =
    std::variant<ValueNone, ValueJSON, ValueSet, ValueString, ValueProperty,
                 ValueStrings, ValueStringSet, ValueTypes, ValueType,
                 ValueRegex, ValueUnsignedInteger, ValueRange, ValueBoolean,
                 ValueNamedIndexes, ValueStringType, ValueStringMap,
                 ValuePropertyFilter, ValueIndexPair, ValuePointer,
                 ValueTypedProperties, ValueTypedHashes>;

} // namespace sourcemeta::blaze

#endif
