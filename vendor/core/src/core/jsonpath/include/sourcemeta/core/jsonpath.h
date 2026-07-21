#ifndef SOURCEMETA_CORE_JSONPATH_H_
#define SOURCEMETA_CORE_JSONPATH_H_

#ifndef SOURCEMETA_CORE_JSONPATH_EXPORT
#include <sourcemeta/core/jsonpath_export.h>
#endif

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonpath_error.h>
// NOLINTEND(misc-include-cleaner)

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/regex.h>

#include <cstdint>    // std::int64_t, std::uint8_t
#include <functional> // std::function
#include <optional>   // std::optional
#include <variant>    // std::variant
#include <vector>     // std::vector

/// @defgroup jsonpath JSONPath
/// @brief A strict RFC 9535 JSONPath implementation.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpath.h>
/// ```

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/// @ingroup jsonpath
/// A parsed RFC 9535 JSONPath query that can be evaluated many times.
class SOURCEMETA_CORE_JSONPATH_EXPORT JSONPath {
public:
  /// The callback invoked for every query result node
  using Callback =
      std::function<void(const JSON &value, const WeakPointer &location)>;

  /// A selector that matches a single object member by name
  struct SelectorName {
    /// The decoded member name
    JSON::String name;
    /// The precomputed object key hash of the member name
    JSON::Object::hash_type hash;
  };

  /// A selector that matches every member or element of a node
  struct SelectorWildcard {};

  /// A selector that matches a single array element by position
  struct SelectorIndex {
    /// The element position, where a negative value counts from the end
    std::int64_t index;
  };

  /// A selector that matches a range of array elements
  struct SelectorSlice {
    /// The position the range starts at, if any
    std::optional<std::int64_t> start;
    /// The position the range stops before, if any
    std::optional<std::int64_t> end;
    /// The distance between selected positions
    std::int64_t step;
  };

  /// The function extensions that filter expressions can invoke
  enum class FilterFunctionName : std::uint8_t {
    /// The length of a string, array, or object value
    Length,
    /// The number of nodes a query selects
    Count,
    /// Whether a string entirely matches a regular expression
    Match,
    /// Whether a string contains a match of a regular expression
    Search,
    /// The value of the single node a query selects
    Value
  };

  /// The operators that filter comparisons can use
  enum class FilterComparisonOperator : std::uint8_t {
    /// Both sides are equal
    Equal,
    /// Both sides are not equal
    NotEqual,
    /// The left side orders before the right side
    Less,
    /// The left side orders before or equals the right side
    LessEqual,
    /// The right side orders before the left side
    Greater,
    /// The right side orders before or equals the left side
    GreaterEqual
  };

#if !defined(DOXYGEN)
  // Required by the recursive grammar and defined further below
  struct Segment;
#endif

  /// A query embedded in a filter expression
  struct FilterQuery {
    /// Whether the query starts at the candidate node instead of the root
    bool relative;
    /// The segments of the query
    std::vector<Segment> segments;
    /// Whether the query selects at most one node
    bool singular;
  };

#if !defined(DOXYGEN)
  // Required by the recursive grammar and defined further below
  struct FilterOperand;
#endif

  /// A function invocation within a filter expression
  struct FilterFunctionCall {
    /// The function to invoke
    FilterFunctionName function;
    /// The arguments to invoke the function with
    std::vector<FilterOperand> arguments;
    /// The compiled form of a constant regular expression argument, which
    /// stays empty when the pattern is not a valid RFC 9485 expression
    std::optional<Regex> compiled;
  };

  /// A comparison side or function argument within a filter expression
  struct FilterOperand {
    /// The literal, query, or function invocation this operand stands for
    std::variant<JSON, FilterQuery, FilterFunctionCall> value;
  };

  /// A comparison between two filter operands
  struct FilterComparison {
    /// The left side of the comparison
    FilterOperand left;
    /// The operator to compare with
    FilterComparisonOperator operation;
    /// The right side of the comparison
    FilterOperand right;
  };

  /// An existence or function test within a filter expression
  struct FilterTest {
    /// Whether the outcome of the test is negated
    bool negated;
    /// The query or function invocation under test
    std::variant<FilterQuery, FilterFunctionCall> subject;
  };

#if !defined(DOXYGEN)
  // Required by the recursive grammar and defined further below
  struct FilterExpression;
#endif

  /// A conjunction of filter expressions
  struct FilterConjunction {
    /// The expressions that must all hold
    std::vector<FilterExpression> children;
  };

  /// A disjunction of filter expressions
  struct FilterDisjunction {
    /// The expressions of which at least one must hold
    std::vector<FilterExpression> children;
  };

  /// A negated parenthesized filter expression
  struct FilterNegation {
    /// The single expression whose outcome is negated
    std::vector<FilterExpression> children;
  };

#if !defined(DOXYGEN)
  // For fast internal dispatching. It must stay in sync with the expression
  // variant below
  enum class FilterExpressionKind : std::uint8_t {
    Comparison = 0,
    Test,
    Conjunction,
    Disjunction,
    Negation
  };
#endif

  /// A logical expression within a filter selector
  struct FilterExpression {
    /// The comparison, test, or combination this expression stands for
    std::variant<FilterComparison, FilterTest, FilterConjunction,
                 FilterDisjunction, FilterNegation>
        value;
  };

  /// A selector that matches the members or elements a logical expression
  /// holds for
  struct SelectorFilter {
    /// The logical expression to apply
    FilterExpression expression;
  };

  /// A single selector within a query segment
  using Selector = std::variant<SelectorName, SelectorWildcard, SelectorIndex,
                                SelectorSlice, SelectorFilter>;

#if !defined(DOXYGEN)
  // For fast internal dispatching. It must stay in sync with the variant above
  enum class SelectorKind : std::uint8_t {
    Name = 0,
    Wildcard,
    Index,
    Slice,
    Filter
  };
#endif

  /// The precomputed shape of a query segment
  enum class SegmentKind : std::uint8_t {
    /// A single name selector applied to children
    SingleName,
    /// A single index selector applied to children
    SingleIndex,
    /// Any other combination of selectors
    General
  };

  /// A step in a query
  struct Segment {
    /// Whether the segment also applies to every descendant of its input
    bool descendant;
    /// The precomputed shape of the segment for fast dispatching
    SegmentKind kind;
    /// The selectors the segment applies
    std::vector<Selector> selectors;
  };

  /// The compiled representation of a whole query
  struct Query {
    /// The segments of the query
    std::vector<Segment> segments;
  };

  /// Parse a query expression, throwing on invalid input. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpath.h>
  ///
  /// const sourcemeta::core::JSONPath path{"$.store.book[0].title"};
  /// ```
  explicit JSONPath(const JSON::StringView expression);

  /// Evaluate the query against a document, invoking the callback once per
  /// result node. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpath.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{
  ///   sourcemeta::core::parse_json("{ \"foo\": [ 1, 2 ] }")};
  /// const sourcemeta::core::JSONPath path{"$.foo[0]"};
  /// path.evaluate(document, [](const auto &value, const auto &location) {
  ///   assert(value.is_integer());
  ///   assert(location.size() == 2);
  /// });
  /// ```
  auto evaluate(const JSON &document, const Callback &callback) const -> void;

  /// Serialize a location into the RFC 9535 normalized path form. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpath.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON document{
  ///   sourcemeta::core::parse_json("{ \"foo\": [ 1, 2 ] }")};
  /// const sourcemeta::core::JSONPath path{"$.foo[0]"};
  /// path.evaluate(document, [](const auto &value, const auto &location) {
  ///   assert(sourcemeta::core::JSONPath::normalize(location) ==
  ///     "$['foo'][0]");
  /// });
  /// ```
  [[nodiscard]] static auto normalize(const WeakPointer &location)
      -> JSON::String;

  /// Serialize the query into its expression string as JSON. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpath.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSONPath path{"$.foo[0]"};
  /// const sourcemeta::core::JSON result{path.to_json()};
  /// assert(result.is_string());
  /// assert(result.to_string() == "$.foo[0]");
  /// ```
  [[nodiscard]] auto to_json() const -> JSON;

  /// Deserialize a query from its expression string as JSON, returning no
  /// result on invalid input. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonpath.h>
  /// #include <cassert>
  ///
  /// const sourcemeta::core::JSON input{"$.foo[0]"};
  /// const auto path{sourcemeta::core::JSONPath::from_json(input)};
  /// assert(path.has_value());
  /// ```
  [[nodiscard]] static auto from_json(const JSON &value)
      -> std::optional<JSONPath>;

private:
  JSON::String expression_;
  Query query_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

#endif
