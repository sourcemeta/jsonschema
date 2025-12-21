#include <sourcemeta/core/json.h>
#include <sourcemeta/core/json_hash.h>
#include <sourcemeta/core/json_value.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/jsonpointer_pointer.h>
#include <sourcemeta/core/uri.h>

#include "grammar.h"
#include "parser.h"
#include "stringify.h"

#include <cassert>     // assert
#include <iterator>    // std::cbegin, std::cend, std::prev, std::advance
#include <memory>      // std::allocator
#include <ostream>     // std::basic_ostream
#include <sstream>     // std::basic_ostringstream, std::basic_stringstream
#include <string>      // std::basic_string
#include <type_traits> // std::is_same_v
#include <utility>     // std::move

namespace {

template <template <typename T> typename Allocator, typename V,
          typename PointerT = sourcemeta::core::GenericPointer<
              typename V::String,
              sourcemeta::core::PropertyHashJSON<typename V::String>>>
auto traverse(V &document, typename PointerT::const_iterator begin,
              typename PointerT::const_iterator end) -> V & {
  // Make sure types match
  static_assert(
      std::is_same_v<typename PointerT::Value, std::remove_const_t<V>>);
  V *current = &document;

  // Evaluation of a JSON Pointer begins with a reference to the root
  // value of a JSON document and completes with a reference to some value
  // within the document.  Each reference token in the JSON Pointer is
  // evaluated sequentially.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  for (auto iterator = begin; iterator != end; ++iterator) {
    if (iterator->is_property()) {
      // If the currently referenced value is a JSON object, the new
      // referenced value is the object member with the name identified by
      // the reference token.  The member name is equal to the token if it
      // has the same number of Unicode characters as the token and their
      // code points are byte-by-byte equal.  No Unicode character
      // normalization is performed.
      // See https://www.rfc-editor.org/rfc/rfc6901#section-4
      current =
          &current->at(iterator->to_property(), iterator->property_hash());
    } else {
      // If the currently referenced value is a JSON array, the reference
      // token MUST contain [...] characters comprised of digits (see ABNF
      // below; note that leading zeros are not allowed) that represent an
      // unsigned base-10 integer value, making the new referenced value the
      // array element with the zero-based index identified by the
      // token.
      // See https://www.rfc-editor.org/rfc/rfc6901#section-4
      if (current->is_object()) {
        current = &current->at(std::to_string(iterator->to_index()));
      } else {
        current = &current->at(iterator->to_index());
      }
    }
  }

  return *current;
}

// A variant of the above function that assumes traversing of
// the entire pointer and does not rely on iterators for performance reasons
template <template <typename T> typename Allocator, typename V,
          typename PointerT = sourcemeta::core::GenericPointer<
              typename V::String,
              sourcemeta::core::PropertyHashJSON<typename V::String>>>
auto traverse_all(V &document, const PointerT &pointer) -> V & {
  // Make sure types match
  static_assert(
      std::is_same_v<typename PointerT::Value, std::remove_const_t<V>>);
  V *current = &document;

  for (const auto &token : pointer) {
    if (token.is_property()) {
      current = &current->at(token.to_property(), token.property_hash());
    } else {
      if (current->is_object()) {
        current = &current->at(std::to_string(token.to_index()));
      } else {
        current = &current->at(token.to_index());
      }
    }
  }

  return *current;
}

template <typename PointerT>
auto try_traverse(const sourcemeta::core::JSON &document,
                  const PointerT &pointer) -> const sourcemeta::core::JSON * {
  const sourcemeta::core::JSON *current = &document;

  for (const auto &token : pointer) {
    const auto type{current->type()};
    const auto is_object{type == sourcemeta::core::JSON::Type::Object};

    if (token.is_property()) {
      if (!is_object) {
        return nullptr;
      }

      const auto &property{token.to_property()};
      const auto *json_value{current->try_at(property, token.property_hash())};
      if (json_value) {
        current = json_value;
      } else {
        return nullptr;
      }
    } else if (type != sourcemeta::core::JSON::Type::Array && !is_object) {
      return nullptr;
    } else {
      const auto index{token.to_index()};
      if (index < current->size()) {
        if (is_object) {
          current = &current->at(std::to_string(index));
        } else {
          current = &current->at(index);
        }
      } else {
        return nullptr;
      }
    }
  }

  return current;
}

} // namespace

namespace sourcemeta::core {

auto get(const JSON &document, const Pointer &pointer) -> const JSON & {
  if (pointer.empty()) {
    return document;
  }

  return traverse_all<std::allocator, const JSON>(document, pointer);
}

auto get(const JSON &document, const WeakPointer &pointer) -> const JSON & {
  if (pointer.empty()) {
    return document;
  }

  return traverse_all<std::allocator, const JSON, WeakPointer>(document,
                                                               pointer);
}

auto get(JSON &document, const Pointer &pointer) -> JSON & {
  if (pointer.empty()) {
    return document;
  }

  return traverse_all<std::allocator, JSON>(document, pointer);
}

auto try_get(const JSON &document, const Pointer &pointer) -> const JSON * {
  return pointer.empty() ? &document : try_traverse(document, pointer);
}

auto try_get(const JSON &document, const WeakPointer &pointer) -> const JSON * {
  return pointer.empty() ? &document : try_traverse(document, pointer);
}

auto get(const JSON &document, const Pointer::Token &token) -> const JSON & {
  if (token.is_property()) {
    return document.at(token.to_property());
  } else {
    return document.at(token.to_index());
  }
}

auto get(const JSON &document, const WeakPointer::Token &token)
    -> const JSON & {
  if (token.is_property()) {
    return document.at(token.to_property());
  } else {
    return document.at(token.to_index());
  }
}

auto get(JSON &document, const Pointer::Token &token) -> JSON & {
  if (token.is_property()) {
    return document.at(token.to_property());
  } else {
    return document.at(token.to_index());
  }
}

auto set(JSON &document, const Pointer &pointer, const JSON &value) -> void {
  if (pointer.empty()) {
    document.into(value);
    return;
  }

  JSON &current{traverse<std::allocator, JSON>(document, std::cbegin(pointer),
                                               std::prev(std::cend(pointer)))};
  const auto &last{pointer.back()};
  // Handle the hyphen as a last constant
  // If the currently referenced value is a JSON array, the reference
  // token [can be ] the single character "-", making the new referenced value
  // the (nonexistent) member after the last array element.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  if (current.is_array() && last.is_hyphen()) {
    current.push_back(value);
  } else if (last.is_property()) {
    current.at(last.to_property()).into(value);
  } else {
    if (current.is_object()) {
      current.at(std::to_string(last.to_index())).into(value);
    } else {
      current.at(last.to_index()).into(value);
    }
  }
}

auto set(JSON &document, const Pointer &pointer, JSON &&value) -> void {
  if (pointer.empty()) {
    document.into(value);
    return;
  }

  JSON &current{traverse<std::allocator, JSON>(document, std::cbegin(pointer),
                                               std::prev(std::cend(pointer)))};
  const auto &last{pointer.back()};
  // Handle the hyphen as a last constant
  // If the currently referenced value is a JSON array, the reference
  // token [can be ] the single character "-", making the new referenced value
  // the (nonexistent) member after the last array element.
  // See https://www.rfc-editor.org/rfc/rfc6901#section-4
  if (current.is_array() && last.is_hyphen()) {
    current.push_back(value);
  } else if (last.is_property()) {
    current.at(last.to_property()).into(std::move(value));
  } else {
    current.at(last.to_index()).into(std::move(value));
  }
}

template <typename PointerT>
auto remove_pointer(JSON &document, const PointerT &pointer) -> bool {
  // Current implementation doesn't support removing an empty JSON pointer
  // because we don't have a reference to the parent JSON object and there may
  // be none (i.e. current object is the root JSON object). Here we are copying
  // RapidJSON by making this a noop.
  // https://github.com/Tencent/rapidjson/blob/24b5e7a8b27f42fa16b96fc70aade9106cf7102f/include/rapidjson/pointer.h#L835C9-L835C55
  if (pointer.empty()) {
    return false;
  }

  JSON &current{traverse<std::allocator, JSON, PointerT>(
      document, std::cbegin(pointer), std::prev(std::cend(pointer)))};
  const auto &last{pointer.back()};

  if (last.is_property()) {
    const auto current_size{current.size()};
    return current.erase(last.to_property()) < current_size;
  } else {
    if (current.is_object()) {
      const auto current_size{current.size()};
      return current.erase(std::to_string(last.to_index())) < current_size;
    } else {
      const auto index{last.to_index()};
      const auto &array{current.as_array()};

      if (index >= array.size()) {
        return false;
      }

      auto iterator{array.cbegin()};
      std::advance(iterator, index);
      current.erase(iterator);
      return true;
    }
  }
}

auto remove(JSON &document, const Pointer &pointer) -> bool {
  return remove_pointer(document, pointer);
}

auto remove(JSON &document, const WeakPointer &pointer) -> bool {
  return remove_pointer(document, pointer);
}

auto to_pointer(const JSON &document) -> Pointer {
  assert(document.is_string());
  auto stream{document.to_stringstream()};
  return parse_pointer(stream);
}

auto to_pointer(const std::basic_string<JSON::Char, JSON::CharTraits,
                                        std::allocator<JSON::Char>> &input)
    -> Pointer {
  std::basic_stringstream<JSON::Char, JSON::CharTraits,
                          std::allocator<JSON::Char>>
      stream;
  stream << internal::token_pointer_quote<JSON::Char>;
  stream << input;
  stream << internal::token_pointer_quote<JSON::Char>;
  return to_pointer(parse_json(stream));
}

auto to_pointer(const WeakPointer &pointer) -> Pointer {
  Pointer result;
  for (const auto &token : pointer) {
    if (token.is_property()) {
      result.push_back(token.to_property());
    } else {
      result.push_back(token.to_index());
    }
  }

  return result;
}

auto to_weak_pointer(const Pointer &pointer) -> WeakPointer {
  WeakPointer result;
  for (const auto &token : pointer) {
    if (token.is_property()) {
      result.push_back(token.to_property());
    } else {
      result.push_back(token.to_index());
    }
  }

  return result;
}

auto stringify(const Pointer &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<JSON::Char, JSON::CharTraits, std::allocator>(pointer, stream);
}

auto stringify(const WeakPointer &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<JSON::Char, JSON::CharTraits, std::allocator>(pointer, stream);
}

auto stringify(const PointerTemplate &pointer,
               std::basic_ostream<JSON::Char, JSON::CharTraits> &stream)
    -> void {
  stringify<JSON::Char, JSON::CharTraits, std::allocator>(pointer, stream);
}

auto to_string(const Pointer &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>> {
  std::basic_ostringstream<JSON::Char, JSON::CharTraits,
                           std::allocator<JSON::Char>>
      result;
  stringify(pointer, result);
  return result.str();
}

auto to_string(const WeakPointer &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>> {
  std::basic_ostringstream<JSON::Char, JSON::CharTraits,
                           std::allocator<JSON::Char>>
      result;
  stringify(pointer, result);
  return result.str();
}

auto to_string(const PointerTemplate &pointer)
    -> std::basic_string<JSON::Char, JSON::CharTraits,
                         std::allocator<JSON::Char>> {
  std::basic_ostringstream<JSON::Char, JSON::CharTraits,
                           std::allocator<JSON::Char>>
      result;
  stringify(pointer, result);
  return result.str();
}

auto to_uri(const Pointer &pointer) -> URI {
  std::basic_ostringstream<JSON::Char, JSON::CharTraits,
                           std::allocator<JSON::Char>>
      result;
  stringify<JSON::Char, JSON::CharTraits, std::allocator>(pointer, result);
  return URI::from_fragment(result.str());
}

auto to_uri(const Pointer &pointer, const URI &base) -> URI {
  return to_uri(pointer).resolve_from(base).canonicalize();
}

} // namespace sourcemeta::core
