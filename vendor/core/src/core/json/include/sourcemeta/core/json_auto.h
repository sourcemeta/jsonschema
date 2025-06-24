#ifndef SOURCEMETA_CORE_JSON_AUTO_H_
#define SOURCEMETA_CORE_JSON_AUTO_H_

#include <sourcemeta/core/json_value.h>

#include <concepts>   // std::same_as, std::constructible_from
#include <functional> // std::function
#include <optional>   // std::optional
#include <tuple> // std::tuple, std::apply, std::tuple_element_t, std::tuple_size, std::tuple_size_v
#include <type_traits> // std::false_type, std::true_type, std::void_t, std::is_enum_v, std::underlying_type_t, std::is_same_v, std::is_base_of_v, std::remove_cvref_t
#include <utility>     // std::pair

namespace sourcemeta::core {

/// @ingroup json
template <typename, typename = void>
struct to_json_has_mapped_type : std::false_type {};
template <typename T>
struct to_json_has_mapped_type<T, std::void_t<typename T::mapped_type>>
    : std::true_type {};

/// @ingroup json
template <typename T> struct to_json_is_basic_string : std::false_type {};
template <typename CharT, typename Traits, typename Alloc>
struct to_json_is_basic_string<std::basic_string<CharT, Traits, Alloc>>
    : std::true_type {};

/// @ingroup json
template <typename T>
concept to_json_has_method = requires(const T value) {
  { value.to_json() } -> std::same_as<JSON>;
};

/// @ingroup json
/// Container-like classes can opt-out from automatic JSON
/// serialisation by setting `using json_auto = std::false_type;`
template <typename, typename = void>
struct to_json_supports_auto_impl : std::true_type {};
template <typename T>
struct to_json_supports_auto_impl<T, std::void_t<typename T::json_auto>>
    : std::bool_constant<
          !std::is_same_v<typename T::json_auto, std::false_type>> {};
template <typename T>
concept to_json_supports_auto = to_json_supports_auto_impl<T>::value;

/// @ingroup json
template <typename T>
concept to_json_list_like =
    requires(T type) {
      typename T::value_type;
      typename T::const_iterator;
      { type.cbegin() } -> std::same_as<typename T::const_iterator>;
      { type.cend() } -> std::same_as<typename T::const_iterator>;
    } && to_json_supports_auto<T> && !to_json_has_mapped_type<T>::value &&
    !to_json_has_method<T> && !to_json_is_basic_string<T>::value;

/// @ingroup json
template <typename T>
concept to_json_map_like =
    requires(T type) {
      typename T::value_type;
      typename T::const_iterator;
      typename T::key_type;
      { type.cbegin() } -> std::same_as<typename T::const_iterator>;
      { type.cend() } -> std::same_as<typename T::const_iterator>;
    } && to_json_supports_auto<T> && to_json_has_mapped_type<T>::value &&
    !to_json_has_method<T> &&
    std::is_same_v<typename T::key_type, JSON::String>;

/// @ingroup json
/// If the value has a `.to_json()` method, always prefer that
template <typename T>
  requires(to_json_has_method<T>)
auto to_json(const T &value) -> JSON {
  return value.to_json();
}

// TODO: How can we keep this in the hash header that does not yet know about
// JSON?
/// @ingroup json
template <typename T>
  requires std::is_same_v<T, JSON::Object::Container::hash_type>
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

/// @ingroup json
template <typename T>
  requires std::constructible_from<JSON, T>
auto to_json(const T &value) -> JSON {
  return JSON{value};
}

/// @ingroup json
template <typename T>
  requires std::is_enum_v<T>
auto to_json(const T value) -> JSON {
  return to_json<std::underlying_type_t<T>>(
      static_cast<std::underlying_type_t<T>>(value));
}

/// @ingroup json
template <typename T> auto to_json(const std::optional<T> &value) -> JSON {
  return value.has_value() ? to_json<T>(value.value()) : JSON{nullptr};
}

/// @ingroup json
template <to_json_list_like T>
auto to_json(typename T::const_iterator begin, typename T::const_iterator end)
    -> JSON {
  // TODO: Extend `make_array` to optionally take iterators, etc
  auto result{JSON::make_array()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.push_back(to_json<typename T::value_type>(*iterator));
  }

  return result;
}

/// @ingroup json
template <to_json_list_like T>
auto to_json(
    typename T::const_iterator begin, typename T::const_iterator end,
    const std::function<JSON(const typename T::value_type &)> &callback)
    -> JSON {
  // TODO: Extend `make_array` to optionally take iterators, etc
  auto result{JSON::make_array()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.push_back(callback(*iterator));
  }

  return result;
}

/// @ingroup json
template <to_json_list_like T> auto to_json(const T &value) -> JSON {
  return to_json<T>(value.cbegin(), value.cend());
}

/// @ingroup json
template <to_json_list_like T>
auto to_json(
    const T &value,
    const std::function<JSON(const typename T::value_type &)> &callback)
    -> JSON {
  return to_json<T>(value.cbegin(), value.cend(), callback);
}

/// @ingroup json
template <to_json_map_like T>
auto to_json(typename T::const_iterator begin, typename T::const_iterator end)
    -> JSON {
  auto result{JSON::make_object()};
  for (auto iterator = begin; iterator != end; ++iterator) {
    result.assign(iterator->first,
                  to_json<typename T::mapped_type>(iterator->second));
  }

  return result;
}

/// @ingroup json
template <to_json_map_like T> auto to_json(const T &value) -> JSON {
  return to_json<T>(value.cbegin(), value.cend());
}

/// @ingroup json
template <to_json_map_like T>
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
template <to_json_map_like T>
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

// Handle 1-element tuples
/// @ingroup json
template <typename T>
  requires(std::tuple_size_v<std::remove_cvref_t<std::tuple<T>>> == 1)
auto to_json(const std::tuple<T> &value) -> JSON {
  auto tuple = JSON::make_array();
  std::apply([&](const T &element) { tuple.push_back(to_json(element)); },
             value);
  return tuple;
}

// We have to do this mess because MSVC seems confuses `std::pair`
// of 2 elements with this overload
/// @ingroup json
template <typename TupleT>
  requires(requires {
    typename std::tuple_size<std::remove_cvref_t<TupleT>>::type;
  } && (std::tuple_size_v<std::remove_cvref_t<TupleT>> >= 2) &&
           (!std::is_base_of_v<
               std::pair<std::tuple_element_t<0, std::remove_cvref_t<TupleT>>,
                         std::tuple_element_t<1, std::remove_cvref_t<TupleT>>>,
               std::remove_cvref_t<TupleT>>))
auto to_json(const TupleT &value) -> JSON {
  auto tuple = JSON::make_array();
  std::apply(
      [&tuple](const auto &...elements) {
        (tuple.push_back(to_json(elements)), ...);
      },
      value);
  return tuple;
}

} // namespace sourcemeta::core

#endif
