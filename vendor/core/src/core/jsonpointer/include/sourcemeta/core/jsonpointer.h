#ifndef SOURCEMETA_CORE_JSONPOINTER_H_
#define SOURCEMETA_CORE_JSONPOINTER_H_

#ifndef SOURCEMETA_CORE_JSONPOINTER_EXPORT
#include <sourcemeta/core/jsonpointer_export.h>
#endif

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/uri.h>

// NOLINTBEGIN(misc-include-cleaner)
#include <sourcemeta/core/jsonpointer_error.h>
#include <sourcemeta/core/jsonpointer_pointer.h>
#include <sourcemeta/core/jsonpointer_position.h>
#include <sourcemeta/core/jsonpointer_template.h>
#include <sourcemeta/core/jsonpointer_walker.h>
// NOLINTEND(misc-include-cleaner)

#include <cassert>     // assert
#include <functional>  // std::reference_wrapper
#include <memory>      // std::allocator
#include <ostream>     // std::basic_ostream
#include <string>      // std::basic_string
#include <string_view> // std::string_view
#include <type_traits> // std::is_same_v

/// @defgroup jsonpointer JSON Pointer
/// @brief A growing implementation of RFC 6901 JSON Pointer.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// ```

namespace sourcemeta::core {

/// @ingroup jsonpointer
using Pointer = GenericPointer<JSON::String, PropertyHashJSON<JSON::String>>;

/// @ingroup jsonpointer
using WeakPointer = GenericPointer<std::reference_wrapper<const std::string>,
                                   PropertyHashJSON<JSON::String>>;

/// @ingroup jsonpointer
/// A global constant instance of the empty JSON Pointer.
const Pointer empty_pointer;

/// @ingroup jsonpointer
/// A global constant instance of the empty JSON WeakPointer.
const WeakPointer empty_weak_pointer;

/// @ingroup jsonpointer
/// A JSON Pointer with unresolved wildcards
using PointerTemplate = GenericPointerTemplate<Pointer>;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON Pointer (`const` overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1 }, { \"bar\": 2 } ]"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// const sourcemeta::core::Pointer pointer{1, "bar"};
/// const sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document, pointer)};
/// assert(value.is_integer());
/// assert(value.to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(const JSON &document, const Pointer &pointer) -> const JSON &;

// Constant reference parameters can accept xvalues which will be destructed
// after the call. When the function returns such a parameter also as constant
// reference, then the returned reference can be used after the object it refers
// to has been destroyed.
// https://clang.llvm.org/extra/clang-tidy/checks/bugprone/return-const-ref-from-parameter.html
// This overload avoids mis-uses of retuning const reference parameter as
// constant reference.
auto get(JSON &&document, const Pointer &pointer) -> const JSON & = delete;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON WeakPointer (`const`
/// overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1 }, { \"bar\": 2 } ]"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// const std::string bar = "bar";
/// const sourcemeta::core::WeakPointer pointer{1, std::cref(bar)};
/// const sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document, pointer)};
/// assert(value.is_integer());
/// assert(value.to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(const JSON &document, const WeakPointer &pointer) -> const JSON &;

// Constant reference parameters can accept xvalues which will be destructed
// after the call. When the function returns such a parameter also as constant
// reference, then the returned reference can be used after the object it refers
// to has been destroyed.
// https://clang.llvm.org/extra/clang-tidy/checks/bugprone/return-const-ref-from-parameter.html
// This overload avoids mis-uses of retuning const reference parameter as
// constant reference.
auto get(JSON &&document, const WeakPointer &pointer) -> const JSON & = delete;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a Pointer, returning an optional that
/// is not set if the path does not exist in the document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1 }, { \"bar\": 2 } ]"};
/// const auto document{sourcemeta::core::parse_json(stream)};
/// const sourcemeta::core::Pointer pointer{1, "bar"};
/// const auto result{sourcemeta::core::try_get(document, pointer)};
/// assert(result);
/// assert(*result == document.at(1).at("bar"));
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto try_get(const JSON &document, const Pointer &pointer) -> const JSON *;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a WeakPointer, returning an optional
/// that is not set if the path does not exist in the document. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1 }, { \"bar\": 2 } ]"};
/// const auto document{sourcemeta::core::parse_json(stream)};
/// const std::string bar = "bar";
/// const sourcemeta::core::WeakPointer pointer{1, std::cref(bar)};
/// const auto result{sourcemeta::core::try_get(document, pointer)};
/// assert(result);
/// assert(*result == document.at(1).at("bar"));
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto try_get(const JSON &document, const WeakPointer &pointer) -> const JSON *;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON Pointer (non-`const`
/// overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1 }, { \"bar\": 2 } ]"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
/// assert(document.at("foo").to_integer() == 1);
///
/// const sourcemeta::core::Pointer pointer{1, "bar"};
/// sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document, pointer)};
/// assert(value.is_integer());
/// assert(value.to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(JSON &document, const Pointer &pointer) -> JSON &;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON Pointer token (`const`
/// overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"{ \"foo\": 1 }"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// const sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document,
///   sourcemeta::core::Pointer{"foo"})};
/// assert(value.is_integer());
/// assert(value.to_integer() == 1);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(const JSON &document, const Pointer::Token &token) -> const JSON &;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON WeakPointer token (`const`
/// overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"{ \"foo\": 1 }"};
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// const std::string foo = "foo";
/// const sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document,
///   sourcemeta::core::WeakPointer{std::cref(foo)})};
/// assert(value.is_integer());
/// assert(value.to_integer() == 1);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(const JSON &document, const WeakPointer::Token &token) -> const JSON &;

/// @ingroup jsonpointer
/// Get a value from a JSON document using a JSON Pointer token (non-`const`
/// overload).
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"{ \"foo\": 1 }"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// sourcemeta::core::JSON &value{
///   sourcemeta::core::get(document, "bar")};
/// assert(value.is_integer());
/// assert(value.to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto get(JSON &document, const Pointer::Token &token) -> JSON &;

/// @ingroup jsonpointer
/// Set a value in a JSON document using a JSON Pointer (`const` overload).
///
/// If the last token of the JSON Pointer is the constant `-` and the tail
/// instance is an array, then the operation is equivalent to
/// `sourcemeta::core::JSON::push_back`.
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"{ \"foo\": 1 }"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
/// assert(document.at("foo").to_integer() == 1);
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// const sourcemeta::core::JSON value{2};
/// sourcemeta::core::set(document, pointer, value);
/// assert(document.at("foo").to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto set(JSON &document, const Pointer &pointer, const JSON &value) -> void;

/// @ingroup jsonpointer
/// Set a value in a JSON document using a JSON Pointer (non-`const` overload).
///
/// If the last token of the JSON Pointer is the constant `-` and the tail
/// instance is an array, then the operation is equivalent to
/// `sourcemeta::core::JSON::push_back`.
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"{ \"foo\": 1 }"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// sourcemeta::core::set(document, pointer,
///   sourcemeta::core::JSON{2});
/// assert(document.at("foo").to_integer() == 2);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto set(JSON &document, const Pointer &pointer, JSON &&value) -> void;

/// @ingroup jsonpointer
/// Remove a value from a JSON document using a JSON Pointer.
/// Returns true if a value is removed, false otherwise.
///
/// Removing an empty pointer `Pointer{}`, i.e. the root, is a noop.
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1, \"baz\": 1 }, { \"bar\": 2 } ]"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
/// assert(document.at(0).defines("foo"));
///
/// const sourcemeta::core::Pointer pointer{0, "foo"};
/// sourcemeta::core::remove(document, pointer);
/// assert(!document.at(0).defines("foo"));
/// assert(document.at(0).defines("baz"));
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto remove(JSON &document, const Pointer &pointer) -> bool;

/// @ingroup jsonpointer
/// Remove a value from a JSON document using a JSON WeakPointer.
/// Returns true if a value is removed, false otherwise.
///
/// Removing an empty pointer `WeakPointer{}`, i.e. the root, is a noop.
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <sstream>
///
/// std::istringstream stream{"[ { \"foo\": 1, \"baz\": 1 }, { \"bar\": 2 } ]"};
/// sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json(stream);
/// assert(document.at(0).defines("foo"));
///
/// const std::string foo = "foo";
/// const sourcemeta::core::WeakPointer pointer{0, std::cref(foo)};
/// sourcemeta::core::remove(document, pointer);
/// assert(!document.at(0).defines("foo"));
/// assert(document.at(0).defines("baz"));
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto remove(JSON &document, const WeakPointer &pointer) -> bool;

/// @ingroup jsonpointer
/// Create a JSON Pointer from a JSON string value. For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const sourcemeta::core::JSON document{"/foo/bar/0"};
/// assert(document.is_string());
/// const sourcemeta::core::Pointer pointer =
///   sourcemeta::core::to_pointer(document);
/// assert(pointer.size() == 3);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_pointer(const JSON &document) -> Pointer;

/// @ingroup jsonpointer
/// Create a JSON Pointer from a standard C++ string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const sourcemeta::core::Pointer pointer =
///   sourcemeta::core::to_pointer("/foo/bar/0");
/// assert(pointer.size() == 3);
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_pointer(const std::basic_string<JSON::Char, JSON::CharTraits,
                                        std::allocator<JSON::Char>> &input)
    -> Pointer;

/// @ingroup jsonpointer
/// Convert a JSON WeakPointer into a JSON Pointer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const std::string foo = "foo";
/// const sourcemeta::core::WeakPointer pointer{std::cref(foo)};
/// const sourcemeta::core::Pointer result{
///   sourcemeta::core::to_pointer(pointer)}:
/// assert(result.size() == 1);
/// assert(result.at(0).is_property());
/// assert(result.at(0).to_property() == "foo");
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_pointer(const WeakPointer &pointer) -> Pointer;

/// @ingroup jsonpointer
/// Convert a JSON Pointer into a JSON WeakPointer. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const sourcemeta::core::Pointer pointer{"foo", "bar", "baz"};
/// const auto result{sourcemeta::core::to_weak_pointer(pointer)};
/// assert(result.size() == 3);
/// assert(result.at(0).is_property());
/// assert(result.at(0).to_property() == "foo");
/// assert(result.at(1).is_property());
/// assert(result.at(1).to_property() == "bar");
/// assert(result.at(2).is_property());
/// assert(result.at(2).to_property() == "baz");
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_weak_pointer(const Pointer &pointer) -> WeakPointer;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer into a given C++ standard output stream.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// std::ostringstream stream;
/// sourcemeta::core::stringify(pointer, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto stringify(const Pointer &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

/// @ingroup jsonpointer
///
/// Stringify the input JSON WeakPointer into a given C++ standard output
/// stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <iostream>
/// #include <sstream>
///
/// const std::string foo = "foo";
/// const sourcemeta::core::WeakPointer pointer{std::cref(foo)};
/// std::ostringstream stream;
/// sourcemeta::core::stringify(pointer, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto stringify(const WeakPointer &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer template into a given C++ standard output
/// stream. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <iostream>
/// #include <sstream>
///
/// const sourcemeta::core::Pointer base{"foo", "bar"};
/// const sourcemeta::core::PointerTemplate pointer{base};
/// std::ostringstream stream;
/// sourcemeta::core::stringify(pointer, stream);
/// std::cout << stream.str() << std::endl;
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto stringify(const PointerTemplate &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer into a C++ standard string. For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <string>
/// #include <iostream>
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// const std::string result{sourcemeta::core::to_string(pointer)};
/// std::cout << result << std::endl;
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_string(const Pointer &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>>;

/// @ingroup jsonpointer
///
/// Stringify the input JSON WeakPointer into a C++ standard string. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <string>
/// #include <iostream>
///
/// const std::string foo = "foo";
/// const sourcemeta::core::WeakPointer pointer{foo};
/// const std::string result{sourcemeta::core::to_string(pointer)};
/// std::cout << result << std::endl;
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_string(const WeakPointer &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>>;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer template into a C++ standard string. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <string>
/// #include <iostream>
///
/// sourcemeta::core::PointerTemplate pointer;
/// pointer.emplace_back(sourcemeta::core::Pointer::Token{"foo"});
/// pointer.emplace_back(sourcemeta::core::PointerTemplate::Wildcard::Property);
/// const std::string result{sourcemeta::core::to_string(pointer)};
/// std::cout << result << '\n';
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_string(const PointerTemplate &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>>;

/// @ingroup jsonpointer
///
/// Mangle a JSON Pointer template and prefix into a collision-free identifier.
///
/// The encoding rules for ASCII characters (0x00-0x7F) are:
///
/// - Lowercase at segment start (except x, u, z): capitalize (no marker)
/// - Lowercase x, u, z at segment start: hex escape (reserved characters)
/// - Uppercase at segment start (except X, U, Z): U + letter
/// - Uppercase X, U, Z at segment start: hex escape (reserved characters)
/// - Non-segment-start lowercase: as-is
/// - Non-segment-start uppercase (except X, U): as-is
/// - Non-segment-start X: X58, Non-segment-start U: X55
/// - ASCII digits (0-9): as-is
/// - Other ASCII (space, punctuation, control): hex escape, starts new segment
/// - Z/z reserved for special token prefixes
///
/// For non-ASCII bytes (0x80-0xFF, e.g. UTF-8 sequences):
///
/// - Always hex escaped
/// - Do NOT start a new segment (preserves UTF-8 multi-byte sequences)
///
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
///
/// const sourcemeta::core::PointerTemplate pointer{"foo", "bar"};
/// const auto result{sourcemeta::core::mangle(pointer, "schema")};
/// assert(result == "Schema_Foo_Bar");
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto mangle(const PointerTemplate &pointer, std::string_view prefix)
    -> std::string;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer into a properly escaped URI fragment. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// #include <sourcemeta/core/jsonpointer.h>
///
/// #include <assert>
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// const sourcemeta::core::URI fragment{
///   sourcemeta::core::to_uri(pointer)};
/// assert(fragment.recompose() == "#/foo");
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_uri(const Pointer &pointer) -> URI;

/// @ingroup jsonpointer
///
/// Stringify the input JSON Pointer into a properly escaped URI fragment
/// alongside a base URI. For example:
///
/// ```cpp
/// #include <sourcemeta/core/uri.h>
/// #include <sourcemeta/core/jsonpointer.h>
///
/// #include <assert>
///
/// const sourcemeta::core::Pointer pointer{"foo"};
/// const sourcemeta::core::URI base{"https://www.example.com"};
/// const sourcemeta::core::URI fragment{
///   sourcemeta::core::to_uri(pointer, base)};
/// assert(fragment.recompose() == "https://example.com#/foo");
/// ```
SOURCEMETA_CORE_JSONPOINTER_EXPORT
auto to_uri(const Pointer &pointer, const URI &base) -> URI;

// TODO: Only support this with weak pointers
/// @ingroup jsonpointer
///
/// Walk over every element of a JSON document, top-down, using JSON Pointers.
/// For example:
///
/// ```cpp
/// #include <sourcemeta/core/json.h>
/// #include <sourcemeta/core/jsonpointer.h>
/// #include <cassert>
/// #include <vector>
///
/// const sourcemeta::core::JSON document =
///   sourcemeta::core::parse_json("[ 1, 2, 3 ]");
/// std::vector<sourcemeta::core::Pointer> subpointers;
///
/// for (const auto &subpointer :
///   sourcemeta::core::PointerWalker{document}) {
///   subpointers.push_back(subpointer);
/// }
///
/// assert(subpointers.size() == 4);
/// assert(subpointers.at(0) == sourcemeta::core::Pointer{});
/// assert(subpointers.at(1) == sourcemeta::core::Pointer{0});
/// assert(subpointers.at(2) == sourcemeta::core::Pointer{1});
/// assert(subpointers.at(3) == sourcemeta::core::Pointer{2});
/// ```
using PointerWalker = GenericPointerWalker<Pointer>;

/// @ingroup jsonpointer
/// Serialise a Pointer as JSON
template <typename T>
  requires std::is_same_v<T, Pointer>
auto to_json(const T &value) -> JSON {
  return JSON{to_string(value)};
}

/// @ingroup jsonpointer
/// Serialise a WeakPointer as JSON
template <typename T>
  requires std::is_same_v<T, WeakPointer>
auto to_json(const T &value) -> JSON {
  return JSON{to_string(value)};
}

/// @ingroup jsonpointer
/// Deserialise a Pointer from JSON
template <typename T>
  requires std::is_same_v<T, Pointer>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_string()) {
    return std::nullopt;
  }

  try {
    return to_pointer(value);
  } catch (const PointerParseError &) {
    return std::nullopt;
  }
}

} // namespace sourcemeta::core

// This hash specialisation is intentationally constant with a decent tolerance
// to collisions
namespace std {
template <typename PropertyT>
struct hash<sourcemeta::core::GenericPointer<
    PropertyT,
    sourcemeta::core::PropertyHashJSON<sourcemeta::core::JSON::String>>> {
  auto
  operator()(const sourcemeta::core::GenericPointer<
             PropertyT,
             sourcemeta::core::PropertyHashJSON<sourcemeta::core::JSON::String>>
                 &pointer) const noexcept -> std::size_t {
    const auto size{pointer.size()};
    if (size == 0) {
      return size;
    }

    const auto &first{pointer.at(0)};
    const auto &middle{pointer.at(size / 2)};
    const auto &last{pointer.at(size - 1)};

    return size +
           (first.is_property()
                ? static_cast<std::size_t>(first.property_hash().a)
                : first.to_index()) +
           (middle.is_property()
                ? static_cast<std::size_t>(middle.property_hash().a)
                : middle.to_index()) +
           (last.is_property()
                ? static_cast<std::size_t>(last.property_hash().a)
                : last.to_index());
  }
};

template <typename PointerT>
struct hash<sourcemeta::core::GenericPointerTemplate<PointerT>> {
  auto operator()(const sourcemeta::core::GenericPointerTemplate<PointerT>
                      &pointer) const noexcept -> std::size_t {
    const auto size{pointer.size()};
    if (size == 0) {
      return size;
    }

    auto hash_element =
        [](const typename sourcemeta::core::GenericPointerTemplate<
            PointerT>::value_type &element) -> std::size_t {
      using Template = sourcemeta::core::GenericPointerTemplate<PointerT>;
      const auto *token{std::get_if<typename Template::Token>(&element)};
      if (token) {
        return token->is_property()
                   ? static_cast<std::size_t>(token->property_hash().a)
                   : token->to_index();
      } else {
        return element.index();
      }
    };

    const auto &first{*pointer.cbegin()};
    const auto &middle{
        *(pointer.cbegin() +
          static_cast<typename sourcemeta::core::GenericPointerTemplate<
              PointerT>::difference_type>(size / 2))};
    const auto &last{*(pointer.cend() - 1)};

    return size + hash_element(first) + hash_element(middle) +
           hash_element(last);
  }
};
} // namespace std

#endif
