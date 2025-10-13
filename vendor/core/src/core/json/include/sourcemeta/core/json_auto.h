#ifndef SOURCEMETA_CORE_JSON_AUTO_H_
#define SOURCEMETA_CORE_JSON_AUTO_H_

#include <sourcemeta/core/json_value.h>

#include <algorithm>  // std::sort
#include <cassert>    // assert
#include <chrono>     // std::chrono
#include <concepts>   // std::same_as, std::constructible_from
#include <filesystem> // std::filesystem
#include <functional> // std::function
#include <optional>   // std::optional, std::nullopt, std::bad_optional_access
#include <tuple> // std::tuple, std::apply, std::tuple_element_t, std::tuple_size, std::tuple_size_v
#include <type_traits> // std::false_type, std::true_type, std::void_t, std::is_enum_v, std::underlying_type_t, std::is_same_v, std::is_base_of_v, std::remove_cvref_t
#include <utility> // std::pair, std:::make_index_sequence, std::index_sequence

namespace sourcemeta::core {

/// @ingroup json
template <typename, typename = void>
struct json_auto_has_mapped_type : std::false_type {};
template <typename T>
struct json_auto_has_mapped_type<T, std::void_t<typename T::mapped_type>>
    : std::true_type {};

/// @ingroup json
template <typename T> struct json_auto_is_basic_string : std::false_type {};
template <typename CharT, typename Traits, typename Alloc>
struct json_auto_is_basic_string<std::basic_string<CharT, Traits, Alloc>>
    : std::true_type {};

/// @ingroup json
template <typename T>
concept json_auto_has_method_from = requires(const JSON &value) {
  { T::from_json(value) } -> std::same_as<std::optional<T>>;
};

/// @ingroup json
template <typename T>
concept json_auto_has_method_to = requires(const T value) {
  { value.to_json() } -> std::same_as<JSON>;
};

/// @ingroup json
/// Container-like classes can opt-out from automatic JSON
/// serialisation by setting `using json_auto = std::false_type;`
template <typename, typename = void>
struct json_auto_supports_auto_impl : std::true_type {};
template <typename T>
struct json_auto_supports_auto_impl<T, std::void_t<typename T::json_auto>>
    : std::bool_constant<
          !std::is_same_v<typename T::json_auto, std::false_type>> {};
template <typename T>
concept json_auto_supports_auto = json_auto_supports_auto_impl<T>::value;

/// @ingroup json
template <typename T>
concept json_auto_list_like =
    requires(T type) {
      typename T::value_type;
      typename T::const_iterator;
      { type.cbegin() } -> std::same_as<typename T::const_iterator>;
      { type.cend() } -> std::same_as<typename T::const_iterator>;
    } && json_auto_supports_auto<T> && !json_auto_has_mapped_type<T>::value &&
    !json_auto_has_method_from<T> && !json_auto_has_method_to<T> &&
    !json_auto_is_basic_string<T>::value;

/// @ingroup json
template <typename T>
concept json_auto_map_like =
    requires(T type) {
      typename T::value_type;
      typename T::const_iterator;
      typename T::key_type;
      { type.cbegin() } -> std::same_as<typename T::const_iterator>;
      { type.cend() } -> std::same_as<typename T::const_iterator>;
    } && json_auto_supports_auto<T> && json_auto_has_mapped_type<T>::value &&
    !json_auto_has_method_from<T> && !json_auto_has_method_to<T> &&
    std::is_same_v<typename T::key_type, JSON::String>;

/// @ingroup json
template <typename, typename = void>
struct json_auto_has_reverse_iterator : std::false_type {};

/// @ingroup json
template <typename T>
struct json_auto_has_reverse_iterator<T,
                                      std::void_t<typename T::reverse_iterator>>
    : std::true_type {};

/// @ingroup json
template <typename T> struct json_auto_is_pair : std::false_type {};

/// @ingroup json
template <typename U, typename V>
struct json_auto_is_pair<std::pair<U, V>> : std::true_type {};

/// @ingroup json
template <typename T>
concept json_auto_tuple_mono = requires {
  typename std::tuple_size<std::remove_cvref_t<T>>::type;
} && (std::tuple_size_v<std::remove_cvref_t<T>> == 1);

// We have to do this mess because MSVC seems confuses `std::pair`
// of 2 elements with this overload
/// @ingroup json
template <typename T>
concept json_auto_tuple_poly =
    requires { typename std::tuple_size<std::remove_cvref_t<T>>::type; } &&
    (std::tuple_size_v<std::remove_cvref_t<T>> >= 2) &&
    (!std::is_base_of_v<
        std::pair<std::tuple_element_t<0, std::remove_cvref_t<T>>,
                  std::tuple_element_t<1, std::remove_cvref_t<T>>>,
        std::remove_cvref_t<T>>);

// Forward declarations for recursive type conversions
#ifndef DOXYGEN
template <json_auto_list_like T> auto to_json(const T &value) -> JSON;
template <json_auto_map_like T> auto to_json(const T &value) -> JSON;
template <typename L, typename R>
auto to_json(const std::pair<L, R> &value) -> JSON;
template <json_auto_tuple_mono T> auto to_json(const T &value) -> JSON;
template <json_auto_tuple_poly T> auto to_json(const T &value) -> JSON;
#endif

/// @ingroup json
/// If the value has a `.to_json()` method, always prefer that
template <typename T>
  requires(json_auto_has_method_to<T>)
auto to_json(const T &value) -> JSON {
  return value.to_json();
}

/// @ingroup json
/// If the value has a `.from_json()` static method, always prefer that
template <typename T>
  requires(json_auto_has_method_from<T>)
auto from_json(const JSON &value) -> std::optional<T> {
  return T::from_json(value);
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, bool>
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_boolean()) {
    return value.to_boolean();
  } else {
    return std::nullopt;
  }
}

/// @ingroup json
template <typename T>
  requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_integer()) {
    return static_cast<T>(value.to_integer());
  } else {
    return std::nullopt;
  }
}

// TODO: How can we keep this in the hash header that does not yet know about
// JSON?
/// @ingroup json
template <typename T>
  requires std::is_same_v<T, JSON::Object::hash_type>
auto to_json(const T &hash) -> JSON {
  auto result{JSON::make_array()};
#if defined(__SIZEOF_INT128__)
  result.push_back(JSON{static_cast<std::size_t>(hash.a >> 64)});
  result.push_back(JSON{static_cast<std::size_t>(hash.a)});
  result.push_back(JSON{static_cast<std::size_t>(hash.b >> 64)});
  result.push_back(JSON{static_cast<std::size_t>(hash.b)});
#else
  result.push_back(JSON{static_cast<std::size_t>(hash.a)});
  result.push_back(JSON{static_cast<std::size_t>(hash.b)});
  result.push_back(JSON{static_cast<std::size_t>(hash.c)});
  result.push_back(JSON{static_cast<std::size_t>(hash.d)});
#endif
  return result;
}

// TODO: How can we keep this in the hash header that does not yet know about
// JSON?
/// @ingroup json
template <typename T>
  requires std::is_same_v<T, JSON::Object::hash_type>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_array() || value.size() != 4 || !value.at(0).is_integer() ||
      !value.at(1).is_integer() || !value.at(2).is_integer() ||
      !value.at(3).is_integer()) {
    return std::nullopt;
  }

#if defined(__SIZEOF_INT128__)
  return T{(static_cast<__uint128_t>(
                static_cast<std::uint64_t>(value.at(0).to_integer()))
            << 64) |
               static_cast<std::uint64_t>(value.at(1).to_integer()),
           (static_cast<__uint128_t>(
                static_cast<std::uint64_t>(value.at(2).to_integer()))
            << 64) |
               static_cast<std::uint64_t>(value.at(3).to_integer())};
#else
  return T{static_cast<std::uint64_t>(value.at(0).to_integer()),
           static_cast<std::uint64_t>(value.at(1).to_integer()),
           static_cast<std::uint64_t>(value.at(2).to_integer()),
           static_cast<std::uint64_t>(value.at(3).to_integer())};
#endif
}

/// @ingroup json
template <typename T>
  requires(std::constructible_from<JSON, T> &&
           // Otherwise MSVC gets confused
           !std::is_same_v<T, unsigned long long>)
auto to_json(const T &value) -> JSON {
  return JSON{value};
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, std::filesystem::file_time_type>
auto to_json(const T value) -> JSON {
  return JSON{static_cast<std::int64_t>(value.time_since_epoch().count())};
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, std::filesystem::file_time_type>
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_integer()) {
    using file_time_type = std::filesystem::file_time_type;
    return file_time_type{file_time_type::duration{
        static_cast<file_time_type::duration::rep>(value.to_integer())}};
  } else {
    return std::nullopt;
  }
}

/// @ingroup json
template <typename T>
  requires(std::is_same_v<T, std::filesystem::path> &&
           // In at least Clang and GCC, paths are convertible to strings,
           // resulting in ambiguous templated calls
           !std::is_convertible_v<T, std::string>)
auto to_json(const T value) -> JSON {
  return JSON{value.string()};
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, std::filesystem::path>
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_string()) {
    return value.to_string();
  } else {
    return std::nullopt;
  }
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, unsigned long long>
auto to_json(const T value) -> JSON {
  return JSON{static_cast<std::int64_t>(value)};
}

/// @ingroup json
template <typename T>
  requires std::is_same_v<T, JSON>
auto from_json(const JSON &value) -> std::optional<T> {
  return value;
}

/// @ingroup json
template <typename T>
  requires json_auto_is_basic_string<T>::value
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_string()) {
    return value.to_string();
  } else {
    return std::nullopt;
  }
}

/// @ingroup json
template <typename T>
  requires std::is_enum_v<T>
auto to_json(const T value) -> JSON {
  return to_json(static_cast<std::underlying_type_t<T>>(value));
}

/// @ingroup json
template <typename T>
  requires std::is_enum_v<T>
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_integer()) {
    return static_cast<T>(value.to_integer());
  } else {
    return std::nullopt;
  }
}

/// @ingroup json
template <typename T> auto to_json(const std::optional<T> &value) -> JSON {
  return value.has_value() ? to_json(value.value()) : JSON{nullptr};
}

/// @ingroup json
template <typename T>
  requires requires { typename T::value_type; } &&
           std::is_same_v<T, std::optional<typename T::value_type>>
auto from_json(const JSON &value) -> std::optional<T> {
  if (value.is_null()) {
    return std::optional<T>{
        std::optional<typename T::value_type>{std::nullopt}};
  } else {
    auto result{from_json<typename T::value_type>(value)};
    if (!result.has_value()) {
      return std::nullopt;
    }

    return result;
  }
}

/// @ingroup json
template <json_auto_list_like T>
auto to_json(typename T::const_iterator begin, typename T::const_iterator end)
    -> JSON {
  // TODO: Extend `make_array` to optionally take iterators, etc
  auto result{JSON::make_array()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.push_back(to_json(*iterator));
  }

  // To guarantee ordering across implementations
  if constexpr (!json_auto_has_reverse_iterator<T>::value) {
    std::sort(result.as_array().begin(), result.as_array().end());
  }

  return result;
}

/// @ingroup json
template <json_auto_list_like T>
auto to_json(
    typename T::const_iterator begin, typename T::const_iterator end,
    const std::function<JSON(const typename T::value_type &)> &callback)
    -> JSON {
  // TODO: Extend `make_array` to optionally take iterators, etc
  auto result{JSON::make_array()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.push_back(callback(*iterator));
  }

  // To guarantee ordering across implementations
  if constexpr (!json_auto_has_reverse_iterator<T>::value) {
    std::sort(result.as_array().begin(), result.as_array().end());
  }

  return result;
}

/// @ingroup json
template <json_auto_list_like T> auto to_json(const T &value) -> JSON {
  return to_json<T>(value.cbegin(), value.cend());
}

/// @ingroup json
template <json_auto_list_like T>
auto to_json(
    const T &value,
    const std::function<JSON(const typename T::value_type &)> &callback)
    -> JSON {
  return to_json<T>(value.cbegin(), value.cend(), callback);
}

/// @ingroup json
template <json_auto_list_like T>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_array()) {
    return std::nullopt;
  }

  T result;

  if constexpr (requires { result.reserve(value.size()); }) {
    result.reserve(value.size());
  }

  for (const auto &item : value.as_array()) {
    auto subvalue{from_json<typename T::value_type>(item)};
    if (!subvalue.has_value()) {
      return std::nullopt;
    }

    if constexpr (requires { result.insert(subvalue.value()); }) {
      result.insert(std::move(subvalue).value());
    } else {
      result.push_back(std::move(subvalue).value());
    }
  }

  return result;
}

/// @ingroup json
template <json_auto_list_like T>
auto from_json(
    const JSON &value,
    const std::function<std::optional<typename T::value_type>(const JSON &)>
        &callback) -> std::optional<T> {
  if (!value.is_array()) {
    return std::nullopt;
  }

  T result;

  if constexpr (requires { result.reserve(value.size()); }) {
    result.reserve(value.size());
  }

  for (const auto &item : value.as_array()) {
    auto subvalue{callback(item)};
    if (!subvalue.has_value()) {
      return std::nullopt;
    }

    if constexpr (requires { result.insert(subvalue.value()); }) {
      result.insert(std::move(subvalue).value());
    } else {
      result.push_back(std::move(subvalue).value());
    }
  }

  return result;
}

/// @ingroup json
template <json_auto_map_like T>
auto to_json(typename T::const_iterator begin, typename T::const_iterator end)
    -> JSON {
  auto result{JSON::make_object()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.assign(iterator->first, to_json(iterator->second));
  }

  return result;
}

/// @ingroup json
template <json_auto_map_like T> auto to_json(const T &value) -> JSON {
  return to_json<T>(value.cbegin(), value.cend());
}

/// @ingroup json
template <json_auto_map_like T>
auto to_json(
    typename T::const_iterator begin, typename T::const_iterator end,
    const std::function<JSON(const typename T::mapped_type &)> &callback)
    -> JSON {
  auto result{JSON::make_object()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.assign(iterator->first, callback(iterator->second));
  }

  return result;
}

/// @ingroup json
template <json_auto_map_like T>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_object()) {
    return std::nullopt;
  }

  T result;
  for (const auto &item : value.as_object()) {
    auto subvalue{from_json<typename T::mapped_type>(item.second)};
    if (!subvalue.has_value()) {
      return std::nullopt;
    }

    result.emplace(item.first, std::move(subvalue).value());
  }

  return result;
}

/// @ingroup json
template <json_auto_map_like T>
auto from_json(
    const JSON &value,
    const std::function<std::optional<typename T::mapped_type>(const JSON &)>
        &callback) -> std::optional<T> {
  if (!value.is_object()) {
    return std::nullopt;
  }

  T result;
  for (const auto &item : value.as_object()) {
    auto subvalue{callback(item.second)};
    if (!subvalue.has_value()) {
      return std::nullopt;
    }

    result.emplace(item.first, std::move(subvalue).value());
  }

  return result;
}

/// @ingroup json
template <json_auto_map_like T>
auto to_json(
    const T &value,
    const std::function<JSON(const typename T::mapped_type &)> &callback)
    -> JSON {
  return to_json<T>(value.cbegin(), value.cend(), callback);
}

/// @ingroup json
template <typename L, typename R>
auto to_json(const std::pair<L, R> &value) -> JSON {
  auto tuple{JSON::make_array()};
  tuple.push_back(to_json(value.first));
  tuple.push_back(to_json(value.second));
  return tuple;
}

/// @ingroup json
template <typename T>
  requires json_auto_is_pair<T>::value
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_array() || value.size() != 2) {
    return std::nullopt;
  }

  auto first{from_json<typename T::first_type>(value.at(0))};
  auto second{from_json<typename T::second_type>(value.at(1))};
  if (!first.has_value() || !second.has_value()) {
    return std::nullopt;
  }

  return std::make_pair<typename T::first_type, typename T::second_type>(
      std::move(first).value(), std::move(second).value());
}

// Handle 1-element tuples
/// @ingroup json
template <json_auto_tuple_mono T> auto to_json(const T &value) -> JSON {
  auto tuple = JSON::make_array();
  std::apply([&](const auto &element) { tuple.push_back(to_json(element)); },
             value);
  return tuple;
}

/// @ingroup json
template <json_auto_tuple_mono T>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_array() || value.size() != 1) {
    return std::nullopt;
  }

  auto first{from_json<std::tuple_element_t<0, T>>(value.at(0))};
  if (!first.has_value()) {
    return std::nullopt;
  }

  return {std::move(first).value()};
}

/// @ingroup json
template <json_auto_tuple_poly T> auto to_json(const T &value) -> JSON {
  auto tuple = JSON::make_array();
  std::apply(
      [&tuple](const auto &...elements) {
        (tuple.push_back(to_json(elements)), ...);
      },
      value);
  return tuple;
}

#ifndef DOXYGEN
template <typename T, std::size_t... Indices>
auto from_json_tuple_poly(const JSON &value, std::index_sequence<Indices...>)
    -> T {
  return {from_json<std::tuple_element_t<Indices, T>>(value.at(Indices))
              .value()...};
}
#endif

/// @ingroup json
template <json_auto_tuple_poly T>
auto from_json(const JSON &value) -> std::optional<T> {
  if (!value.is_array() || value.size() != std::tuple_size_v<T>) {
    return std::nullopt;
  }

  try {
    return from_json_tuple_poly<T>(
        value, std::make_index_sequence<std::tuple_size_v<T>>{});
    // TODO: Maybe there is a better way to catch this without using exceptions?
  } catch (const std::bad_optional_access &) {
    return std::nullopt;
  }
}

} // namespace sourcemeta::core

#endif
