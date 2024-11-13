#include <sourcemeta/jsontoolkit/json_value.h>

#include <algorithm> // std::find
#include <cassert>   // assert
#include <cmath>     // std::isinf, std::isnan, std::modf, std::trunc
#include <numeric>   // std::transform
#include <stdexcept> // std::invalid_argument
#include <string>    // std::to_string
#include <utility>   // std::move
#include <variant>   // std::holds_alternative, std::get

namespace sourcemeta::jsontoolkit {

JSON::JSON(const std::int64_t value)
    : data{std::in_place_type<Integer>, value} {}

JSON::JSON(const std::size_t value)
    : data{std::in_place_type<Integer>, value} {}

JSON::JSON(const int value) : data{std::in_place_type<Integer>, value} {}

JSON::JSON(const double value) : data{std::in_place_type<Real>, value} {
  // Numeric values that cannot be represented as sequences of digits (such as
  // Infinity and NaN) are not permitted. See
  // https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
  if (std::isinf(value) || std::isnan(value)) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }
}

JSON::JSON(const float value) : JSON(static_cast<Real>(value)) {}

JSON::JSON(const bool value) : data{std::in_place_type<bool>, value} {}

JSON::JSON(const std::nullptr_t)
    : data{std::in_place_type<std::nullptr_t>, nullptr} {}

JSON::JSON(const String &value) : data{std::in_place_type<String>, value} {}

JSON::JSON(const std::basic_string_view<Char, CharTraits> &value)
    : data{std::in_place_type<String>, value} {}

JSON::JSON(const Char *const value) : data{std::in_place_type<String>, value} {}

JSON::JSON(std::initializer_list<JSON> values)
    : data{std::in_place_type<Array>, values} {
// For some reason, if we construct a JSON by passing a single
// JSON as argument, GCC and MSVC, in some circumstances will
// prefer this initializer list constructor over the default copy constructor,
// effectively creating an array of a single element. We couldn't find a nicer
// way to force them to pick the correct constructor. This is a hacky (and
// potentially inefficient?) way to "fix it up" to get consistent behavior
// across compilers.
#if defined(__GNUC__) || defined(_MSC_VER)
  if (values.size() == 1) {
    this->data = values.begin()->data;
  }
#endif
}

JSON::JSON(const Array &value) : data{std::in_place_type<Array>, value} {}

JSON::JSON(std::initializer_list<typename Object::Container::value_type> values)
    : data{std::in_place_type<Object>, values} {}

JSON::JSON(const Object &value) : data{std::in_place_type<Object>, value} {}

auto JSON::make_array() -> JSON { return JSON{Array{}}; }

auto JSON::make_object() -> JSON { return JSON{Object{}}; }

auto JSON::size(const String &value) noexcept -> std::size_t {
  std::size_t result{0};

  // We want to count the number of logical characters,
  // not the number of bytes
  for (const auto character : value) {
    // In UTF-8, continuation bytes (i.e. not the first) are
    // encoded as `10xxxxxx`, so this means we are at the start
    // of a code-point
    // See https://en.wikipedia.org/wiki/UTF-8#Encoding
    if ((character & 0b11000000) != 0b10000000) {
      result += 1;
    }
  }

  return result;
}

auto JSON::operator<(const JSON &other) const noexcept -> bool {
  if ((this->type() == Type::Integer && other.type() == Type::Real) ||
      (this->type() == Type::Real && other.type() == Type::Integer)) {
    return this->as_real() < other.as_real();
  }

  if (this->type() != other.type()) {
    return this->data.index() < other.data.index();
  }

  switch (this->type()) {
    case Type::Null:
      return false;
    case Type::Boolean:
      return this->to_boolean() < other.to_boolean();
    case Type::Integer:
      return this->to_integer() < other.to_integer();
    case Type::Real:
      return this->to_real() < other.to_real();
    case Type::String:
      return this->to_string() < other.to_string();
    case Type::Array:
      return this->as_array() < other.as_array();
    case Type::Object:
      return this->as_object() < other.as_object();
    default:
      return false;
  }
}

auto JSON::operator<=(const JSON &other) const noexcept -> bool {
  return *this < other || *this == other;
}

auto JSON::operator>(const JSON &other) const noexcept -> bool {
  return !(*this < other) && *this != other;
}

auto JSON::operator>=(const JSON &other) const noexcept -> bool {
  return *this > other || *this == other;
}

auto JSON::operator==(const JSON &other) const noexcept -> bool {
  if ((this->type() == Type::Integer && other.type() == Type::Real) ||
      (this->type() == Type::Real && other.type() == Type::Integer)) {
    return this->as_real() == other.as_real();
  }

  return this->data == other.data;
}

auto JSON::operator+(const JSON &other) const -> JSON {
  assert(this->is_number());
  assert(other.is_number());

  if (this->is_integer() && other.is_integer()) {
    return JSON{this->to_integer() + other.to_integer()};
  } else if (this->is_integer() && other.is_real()) {
    return JSON{this->as_real() + other.to_real()};
  } else if (this->is_real() && other.is_integer()) {
    return JSON{this->to_real() + other.as_real()};
  } else {
    return JSON{this->to_real() + other.to_real()};
  }
}

auto JSON::operator-(const JSON &other) const -> JSON {
  assert(this->is_number());
  assert(other.is_number());

  if (this->is_integer() && other.is_integer()) {
    return JSON{this->to_integer() - other.to_integer()};
  } else if (this->is_integer() && other.is_real()) {
    return JSON{this->as_real() - other.to_real()};
  } else if (this->is_real() && other.is_integer()) {
    return JSON{this->to_real() - other.as_real()};
  } else {
    return JSON{this->to_real() - other.to_real()};
  }
}

auto JSON::operator+=(const JSON &additive) -> JSON & {
  return *this = *this + additive;
}

auto JSON::operator-=(const JSON &substractive) -> JSON & {
  return *this = *this - substractive;
}

[[nodiscard]] auto JSON::is_boolean() const noexcept -> bool {
  return std::holds_alternative<bool>(this->data);
}

[[nodiscard]] auto JSON::is_null() const noexcept -> bool {
  return std::holds_alternative<std::nullptr_t>(this->data);
}

[[nodiscard]] auto JSON::is_integer() const noexcept -> bool {
  return std::holds_alternative<Integer>(this->data);
}

[[nodiscard]] auto JSON::is_real() const noexcept -> bool {
  return std::holds_alternative<Real>(this->data);
}

[[nodiscard]] auto JSON::is_integer_real() const noexcept -> bool {
  if (this->is_integer()) {
    return false;
  } else if (this->is_real()) {
    const auto value{this->to_real()};
    Real integral;
    return !std::isinf(value) && !std::isnan(value) &&
           std::modf(value, &integral) == 0.0;
  } else {
    return false;
  }
}

[[nodiscard]] auto JSON::is_number() const noexcept -> bool {
  return this->is_integer() || this->is_real();
}

[[nodiscard]] auto JSON::is_positive() const noexcept -> bool {
  switch (this->type()) {
    case Type::Integer:
      return this->to_integer() >= 0;
    case Type::Real:
      return this->to_real() >= static_cast<Real>(0.0);
    default:
      return false;
  }
}

[[nodiscard]] auto JSON::is_string() const noexcept -> bool {
  return std::holds_alternative<JSON::String>(this->data);
}

[[nodiscard]] auto JSON::is_array() const noexcept -> bool {
  return std::holds_alternative<JSON::Array>(this->data);
}

[[nodiscard]] auto JSON::is_object() const noexcept -> bool {
  return std::holds_alternative<Object>(this->data);
}

[[nodiscard]] auto JSON::type() const noexcept -> Type {
  return static_cast<Type>(this->data.index());
}

[[nodiscard]] auto JSON::to_boolean() const noexcept -> bool {
  assert(this->is_boolean());
  return std::get<bool>(this->data);
}

[[nodiscard]] auto JSON::to_integer() const noexcept -> Integer {
  assert(this->is_integer());
  return std::get<Integer>(this->data);
}

[[nodiscard]] auto JSON::to_real() const noexcept -> Real {
  assert(this->is_real());
  return std::get<Real>(this->data);
}

[[nodiscard]] auto JSON::to_string() const noexcept -> const JSON::String & {
  assert(this->is_string());
  return std::get<JSON::String>(this->data);
}

[[nodiscard]] auto JSON::to_string() noexcept -> JSON::String & {
  assert(this->is_string());
  return std::get<JSON::String>(this->data);
}

[[nodiscard]] auto JSON::to_stringstream() const
    -> std::basic_istringstream<Char, CharTraits, Allocator<Char>> {
  return std::basic_istringstream<Char, CharTraits, Allocator<Char>>{
      std::get<JSON::String>(this->data)};
}

[[nodiscard]] auto JSON::as_array() const noexcept -> const JSON::Array & {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data);
}

[[nodiscard]] auto JSON::as_array() noexcept -> JSON::Array & {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data);
}

[[nodiscard]] auto JSON::as_object() noexcept -> Object & {
  assert(this->is_object());
  return std::get<Object>(this->data);
}

[[nodiscard]] auto JSON::as_object() const noexcept -> const Object & {
  assert(this->is_object());
  return std::get<Object>(this->data);
}

[[nodiscard]] auto JSON::as_real() const noexcept -> Real {
  assert(this->is_number());
  return this->is_real() ? this->to_real()
                         : static_cast<Real>(this->to_integer());
}

[[nodiscard]] auto JSON::as_integer() const noexcept -> Integer {
  assert(this->is_number());
  if (this->is_integer()) {
    return this->to_integer();
  } else {
    return static_cast<Integer>(std::trunc(this->to_real()));
  }
}

[[nodiscard]] auto JSON::at(const typename JSON::Array::size_type index) const
    -> const JSON & {
  assert(this->is_array());
  assert(index < this->size());
  return std::get<JSON::Array>(this->data).data.at(index);
}

[[nodiscard]] auto JSON::at(const typename JSON::Array::size_type index)
    -> JSON & {
  assert(this->is_array());
  assert(index < this->size());
  return std::get<JSON::Array>(this->data).data.at(index);
}

[[nodiscard]] auto JSON::at(const JSON::String &key) const -> const JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  return std::get<Object>(this->data).data.find(key)->second;
}

[[nodiscard]] auto JSON::at(const JSON::String &key) -> JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  return std::get<Object>(this->data).data.find(key)->second;
}

[[nodiscard]] auto JSON::front() -> JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return std::get<JSON::Array>(this->data).data.front();
}

[[nodiscard]] auto JSON::front() const -> const JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return std::get<JSON::Array>(this->data).data.front();
}

[[nodiscard]] auto JSON::back() -> JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return std::get<JSON::Array>(this->data).data.back();
}

[[nodiscard]] auto JSON::back() const -> const JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return std::get<JSON::Array>(this->data).data.back();
}

[[nodiscard]] auto JSON::size() const -> std::size_t {
  if (this->is_object()) {
    return std::get<Object>(this->data).data.size();
  } else if (this->is_array()) {
    return std::get<JSON::Array>(this->data).data.size();
  } else {
    assert(this->is_string());
    return JSON::size(std::get<JSON::String>(this->data));
  }
}

[[nodiscard]] auto JSON::byte_size() const -> std::size_t {
  assert(this->is_string());
  return std::get<JSON::String>(this->data).size();
}

[[nodiscard]] auto JSON::estimated_byte_size() const -> std::uint64_t {
  // Of course, container have some overhead of their own
  // which we are not taking into account here, as its typically
  // implementation dependent. This function is just a rough estimate.
  if (this->is_object()) {
    return std::accumulate(this->as_object().cbegin(), this->as_object().cend(),
                           static_cast<std::uint64_t>(0),
                           [](const std::uint64_t accumulator,
                              const typename Object::value_type &pair) {
                             return accumulator +
                                    (pair.first.size() * sizeof(Char)) +
                                    pair.second.estimated_byte_size();
                           });
  } else if (this->is_array()) {
    return std::accumulate(
        this->as_array().cbegin(), this->as_array().cend(),
        static_cast<std::uint64_t>(0),
        [](const std::uint64_t accumulator, const JSON &item) {
          return accumulator + item.estimated_byte_size();
        });
  } else if (this->is_string()) {
    // Keep in mind that standard strings might reserve more
    // space than what it is actually used by the string
    return this->byte_size() * sizeof(Char);
  } else if (this->is_integer()) {
    return sizeof(Integer);
  } else if (this->is_real()) {
    return sizeof(Real);
  } else if (this->is_boolean()) {
    return sizeof(bool);
  } else {
    return sizeof(std::nullptr_t);
  }
}

[[nodiscard]] auto JSON::divisible_by(const JSON &divisor) const -> bool {
  assert(this->is_number());
  assert(divisor.is_number());

  if (this->is_integer() && divisor.is_integer()) {
    const auto divisor_value{divisor.to_integer()};
    return divisor_value != 0 && this->to_integer() % divisor_value == 0;
  }

  const auto divisor_value(divisor.as_real());
  if (divisor_value == 0.0) {
    return false;
  }

  const auto dividend_value{this->as_real()};

  // Every real number that represents an integral is divisible by 0.5.
  Real dividend_integral;
  if (std::modf(dividend_value, &dividend_integral) == 0.0 &&
      divisor_value == 0.5) {
    return true;
  }

  const auto division{dividend_value / divisor_value};
  Real integral;
  return !std::isinf(division) && !std::isnan(division) &&
         std::modf(division, &integral) == 0.0;
}

[[nodiscard]] auto JSON::empty() const -> bool {
  if (this->is_object()) {
    return std::get<Object>(this->data).data.empty();
  } else if (this->is_array()) {
    return std::get<JSON::Array>(this->data).data.empty();
  } else {
    return std::get<JSON::String>(this->data).empty();
  }
}

[[nodiscard]] auto JSON::try_at(const JSON::String &key) const
    -> std::optional<std::reference_wrapper<const JSON>> {
  assert(this->is_object());

  const auto &object{std::get<Object>(this->data)};
  const auto value{object.data.find(key)};

  if (value == object.data.cend()) {
    return std::nullopt;
  }
  return value->second;
}

[[nodiscard]] auto JSON::defines(const JSON::String &key) const -> bool {
  assert(this->is_object());
  return std::get<Object>(this->data).data.contains(key);
}

[[nodiscard]] auto
JSON::defines(const typename JSON::Array::size_type index) const -> bool {
  return this->defines(std::to_string(index));
}

[[nodiscard]] auto
JSON::defines_any(std::initializer_list<JSON::String> keys) const -> bool {
  return this->defines_any(keys.begin(), keys.end());
}

[[nodiscard]] auto JSON::contains(const JSON &element) const -> bool {
  assert(this->is_array());
  return std::find(this->as_array().cbegin(), this->as_array().cend(),
                   element) != this->as_array().cend();
}

[[nodiscard]] auto JSON::unique() const -> bool {
  assert(this->is_array());
  const auto &items{std::get<JSON::Array>(this->data).data};
  // Arrays of 0 or 1 item are unique by definition
  if (items.size() <= 1) {
    return true;
  }

  // Otherwise std::unique would require us to create a copy of the contents
  for (auto iterator = items.cbegin(); iterator != items.cend(); ++iterator) {
    for (auto subiterator = std::next(iterator); subiterator != items.cend();
         ++subiterator) {
      if (*iterator == *subiterator) {
        return false;
      }
    }
  }

  return true;
}

auto JSON::push_back(const JSON &value) -> void {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data).data.push_back(value);
}

auto JSON::push_back(JSON &&value) -> void {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data).data.push_back(std::move(value));
}

auto JSON::push_back_if_unique(const JSON &value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  assert(this->is_array());
  auto &array_data{this->as_array().data};
  const auto match{std::find(array_data.cbegin(), array_data.cend(), value)};
  if (match == array_data.cend()) {
    array_data.push_back(value);
    return {array_data.back(), true};
  } else {
    return {*match, false};
  }
}

auto JSON::push_back_if_unique(JSON &&value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  assert(this->is_array());
  auto &array_data{this->as_array().data};
  const auto match{std::find(array_data.cbegin(), array_data.cend(), value)};
  if (match == array_data.cend()) {
    array_data.push_back(std::move(value));
    return {array_data.back(), true};
  } else {
    return {*match, false};
  }
}

auto JSON::assign(const JSON::String &key, const JSON &value) -> void {
  assert(this->is_object());
  std::get<Object>(this->data).data.insert_or_assign(key, value);
}

auto JSON::assign(const JSON::String &key, JSON &&value) -> void {
  assert(this->is_object());
  std::get<Object>(this->data).data.insert_or_assign(key, std::move(value));
}

auto JSON::assign_if_missing(const JSON::String &key, const JSON &value)
    -> void {
  assert(this->is_object());
  if (!this->defines(key)) {
    this->assign(key, value);
  }
}

auto JSON::assign_if_missing(const JSON::String &key, JSON &&value) -> void {
  assert(this->is_object());
  if (!this->defines(key)) {
    this->assign(key, std::move(value));
  }
}

auto JSON::erase(const JSON::String &key) -> typename Object::size_type {
  assert(this->is_object());
  return std::get<Object>(this->data).data.erase(key);
}

auto JSON::erase_keys(std::initializer_list<JSON::String> keys) -> void {
  this->erase_keys(keys.begin(), keys.end());
}

auto JSON::erase(typename JSON::Array::const_iterator position) ->
    typename JSON::Array::iterator {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data).data.erase(position);
}

auto JSON::erase(typename JSON::Array::const_iterator first,
                 typename JSON::Array::const_iterator last) ->
    typename JSON::Array::iterator {
  assert(this->is_array());
  return std::get<JSON::Array>(this->data).data.erase(first, last);
}

auto JSON::clear() -> void {
  if (this->is_object()) {
    std::get<Object>(this->data).data.clear();
  } else {
    std::get<JSON::Array>(this->data).data.clear();
  }
}

auto JSON::clear_except(std::initializer_list<JSON::String> keys) -> void {
  this->clear_except(keys.begin(), keys.end());
}

auto JSON::into(const JSON &other) -> void { this->data = other.data; }

auto JSON::into(JSON &&other) noexcept -> void {
  this->data = std::move(other.data);
}

auto JSON::into_array() -> void { this->into(JSON::make_array()); }

auto JSON::into_object() -> void { this->into(JSON::make_object()); }

} // namespace sourcemeta::jsontoolkit
