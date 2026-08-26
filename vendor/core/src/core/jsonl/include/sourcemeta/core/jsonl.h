#ifndef SOURCEMETA_CORE_JSONL_H_
#define SOURCEMETA_CORE_JSONL_H_

#ifndef SOURCEMETA_CORE_JSONL_EXPORT
#include <sourcemeta/core/jsonl_export.h>
#endif

#include <sourcemeta/core/json.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonl_iterator.h>
// NOLINTEND(misc-include-cleaner)

#include <cstdint> // std::uint8_t
#include <istream> // std::basic_istream
#include <memory>  // std::unique_ptr

/// @defgroup jsonl JSONL
/// @brief A JSON Lines (https://jsonlines.org) implementation with iterator
/// support. Every non-empty line in a JSONL stream is a complete, valid JSON
/// value of any type, and lines are separated by newline characters (U+000A),
/// optionally preceded by a carriage return (U+000D). Multi-line JSON values
/// are not supported, as per the JSONL specification.
///
/// JSON Lines and NDJSON (https://github.com/ndjson/ndjson-spec) describe the
/// same format with minor differences, and this implementation accepts a
/// superset of both:
///
/// - Blank and whitespace-only lines are skipped rather than treated as
///   errors. JSON Lines considers them invalid, while NDJSON 3.2 permits
///   ignoring them as long as the behavior is documented
/// - A newline after the last value is optional. JSON Lines makes it a
///   recommendation, while NDJSON 3.1 requires it when serializing
/// - Whitespace is tolerated anywhere around a value, including carriage
///   returns that NDJSON 3.1 only allows right before a newline
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonl.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonl
/// A range over the JSON documents contained in a JSON Lines stream.
class SOURCEMETA_CORE_JSONL_EXPORT JSONL {
public:
  /// The mode of operation for the JSONL parser
  enum class Mode : std::uint8_t {
    /// The input stream contains raw JSONL text
    Raw,
    /// The input stream contains gzip-compressed JSONL data
    GZIP
  };

  /// Parse a JSONL document from a C++ standard input stream using a standard
  /// read-only C++ forward iterator interface. An optional mode parameter
  /// controls whether the input is treated as raw text or gzip-compressed
  /// data. For example, you can parse a JSONL document and prettify each of
  /// its rows as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/core/jsonl.h>
  /// #include <cassert>
  /// #include <sstream>
  /// #include <iostream>
  ///
  /// std::istringstream stream{
  ///   "{ \"foo\": 1 }\n{ \"bar\": 2 }\n{ \"baz\": 3 }"};
  ///
  /// for (const auto &document : sourcemeta::core::JSONL{stream}) {
  ///   assert(document.is_object());
  ///   sourcemeta::core::prettify(document, std::cout);
  ///   std::cout << '\n';
  /// }
  /// ```
  ///
  /// If parsing fails, sourcemeta::core::JSONParseError will be thrown.
  JSONL(std::basic_istream<JSON::Char, JSON::CharTraits> &input,
        Mode mode = Mode::Raw);
  ~JSONL();

  JSONL(const JSONL &) = delete;
  auto operator=(const JSONL &) -> JSONL & = delete;
  JSONL(JSONL &&) = delete;
  auto operator=(JSONL &&) -> JSONL & = delete;

  using const_iterator = ConstJSONLIterator;
  auto begin() -> const_iterator;
  auto end() -> const_iterator;
  auto cbegin() -> const_iterator;
  auto cend() -> const_iterator;

private:
// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
  std::basic_istream<JSON::Char, JSON::CharTraits> *stream_;
  struct Internal;
  std::unique_ptr<Internal> internal_;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
};

} // namespace sourcemeta::core

#endif
