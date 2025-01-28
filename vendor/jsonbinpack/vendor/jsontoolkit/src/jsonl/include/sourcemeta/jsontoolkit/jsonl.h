#ifndef SOURCEMETA_JSONTOOLKIT_JSONL_H_
#define SOURCEMETA_JSONTOOLKIT_JSONL_H_

#ifndef SOURCEMETA_JSONTOOLKIT_JSONL_EXPORT
#include <sourcemeta/jsontoolkit/jsonl_export.h>
#endif

#include <sourcemeta/jsontoolkit/json.h>
#include <sourcemeta/jsontoolkit/jsonl_iterator.h>

#include <istream> // std::basic_istream

/// @defgroup jsonl JSONL
/// @brief A JSON Lines implementation with iterators support.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/jsontoolkit/jsonl.h>
/// ```

namespace sourcemeta::jsontoolkit {

/// @ingroup jsonl
class SOURCEMETA_JSONTOOLKIT_JSONL_EXPORT JSONL {
public:
  /// Parse a JSONL document from a C++ standard input stream using a standard
  /// read-only C++ forward iterator interface. For example, you can parse a
  /// JSONL document and prettify each of its rows as follows:
  ///
  /// ```cpp
  /// #include <sourcemeta/jsontoolkit/jsonl.h>
  /// #include <cassert>
  /// #include <sstream>
  /// #include <iostream>
  ///
  /// std::istringstream stream{
  ///   "{ \"foo\": 1 }\n{ \"bar\": 2 }\n{ \"baz\": 3 }"};
  ///
  /// for (const auto &document : sourcemeta::jsontoolkit::JSONL{stream}) {
  ///   assert(document.is_object());
  ///   sourcemeta::jsontoolkit::prettify(document, std::cout);
  ///   std::cout << '\n';
  /// }
  /// ```
  ///
  /// If parsing fails, sourcemeta::jsontoolkit::ParseError will be thrown.
  JSONL(std::basic_istream<JSON::Char, JSON::CharTraits> &stream);

  using const_iterator = ConstJSONLIterator;
  auto begin() -> const_iterator;
  auto end() -> const_iterator;
  auto cbegin() -> const_iterator;
  auto cend() -> const_iterator;

private:
  std::basic_istream<JSON::Char, JSON::CharTraits> &data;
};

} // namespace sourcemeta::jsontoolkit

#endif
