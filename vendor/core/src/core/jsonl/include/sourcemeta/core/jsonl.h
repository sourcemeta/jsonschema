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
/// support. Each line in a JSONL stream must be a complete, valid JSON value.
/// Lines are separated by newline characters (U+000A). Multi-line JSON values
/// are not supported, as per the JSONL specification.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonl.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonl
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
  JSONL(std::basic_istream<JSON::Char, JSON::CharTraits> &stream,
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
#pragma warning(disable : 4251)
#endif
  std::basic_istream<JSON::Char, JSON::CharTraits> *stream;
  struct Internal;
  std::unique_ptr<Internal> internal;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::core

#endif
