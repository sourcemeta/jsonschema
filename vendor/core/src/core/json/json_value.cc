#include <sourcemeta/core/json_array.h>
#include <sourcemeta/core/json_value.h>

#include <algorithm>        // std::find
#include <cassert>          // assert
#include <cmath>            // std::isinf, std::isnan, std::modf, std::trunc
#include <cstddef>          // std::size_t
#include <cstdint>          // std::int64_t
#include <functional>       // std::reference_wrapper
#include <initializer_list> // std::initializer_list
#include <numeric>          // std::transform
#include <sstream>          // std::basic_istringstream
#include <stdexcept>        // std::invalid_argument
#include <string>           // std::to_string
#include <string_view>      // std::basic_string_view
#include <utility>          // std::move
#include <vector>           // std::vector

namespace sourcemeta::core {

static constexpr auto TRIM_WHITESPACE = " \t\n\r\v\f";

JSON::JSON(const std::int64_t value) : current_type{Type::Integer} {
  this->data_integer = value;
}

JSON::JSON(const std::size_t value) : current_type{Type::Integer} {
  this->data_integer = static_cast<Integer>(value);
}

JSON::JSON(const int value) : current_type{Type::Integer} {
  this->data_integer = value;
}

JSON::JSON(const double value) : current_type{Type::Real} {
  // Numeric values that cannot be represented as sequences of digits (such as
  // Infinity and NaN) are not permitted. See
  // https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
  if (std::isinf(value) || std::isnan(value)) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  this->data_real = value;
}

JSON::JSON(const float value) : JSON(static_cast<Real>(value)) {}

JSON::JSON(const bool value) : current_type{Type::Boolean} {
  this->data_boolean = value;
}

JSON::JSON(const std::nullptr_t) {}

JSON::JSON(const String &value) : current_type{Type::String} {
  new (&this->data_string) String{value};
}

JSON::JSON(const std::basic_string_view<Char, CharTraits> &value)
    : current_type{Type::String} {
  new (&this->data_string) String{value};
}

JSON::JSON(const Char *const value) : current_type{Type::String} {
  new (&this->data_string) String{value};
}

JSON::JSON(std::initializer_list<JSON> values) : current_type{Type::Array} {
  new (&this->data_array) Array{values};

// For some reason, if we construct a JSON by passing a single
// JSON as argument, GCC and MSVC, in some circumstances will
// prefer this initializer list constructor over the default copy constructor,
// effectively creating an array of a single element. We couldn't find a nicer
// way to force them to pick the correct constructor. This is a hacky (and
// potentially inefficient?) way to "fix it up" to get consistent behavior
// across compilers.
#if defined(__GNUC__) || defined(_MSC_VER)
  if (values.size() == 1) {
    this->operator=(*values.begin());
  }
#endif
}

JSON::JSON(const Array &value) : current_type{Type::Array} {
  new (&this->data_array) Array{value};
}

JSON::JSON(std::initializer_list<typename Object::pair_value_type> values)
    : current_type{Type::Object} {
  new (&this->data_object) Object{values};
}

JSON::JSON(const Object &value) : current_type{Type::Object} {
  new (&this->data_object) Object{value};
}

JSON::JSON(const Decimal &value) : current_type{Type::Decimal} {
  if (value.is_nan() || value.is_infinite()) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  this->data_decimal = new Decimal{value};
}

JSON::JSON(Decimal &&value) : current_type{Type::Decimal} {
  if (value.is_nan() || value.is_infinite()) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  this->data_decimal = new Decimal{std::move(value)};
}

JSON::JSON(const JSON &other) : current_type{other.current_type} {
  switch (other.current_type) {
    case Type::Boolean:
      this->data_boolean = other.data_boolean;
      break;
    case Type::Integer:
      this->data_integer = other.data_integer;
      break;
    case Type::Real:
      this->data_real = other.data_real;
      break;
    case Type::String:
      new (&this->data_string) String{other.data_string};
      break;
    case Type::Array:
      new (&this->data_array) Array{other.data_array};
      break;
    case Type::Object:
      new (&this->data_object) Object{other.data_object};
      break;
    case Type::Decimal:
      this->data_decimal = new Decimal{*other.data_decimal};
      break;
    default:
      break;
  }
}

JSON::JSON(JSON &&other) noexcept : current_type{other.current_type} {
  switch (other.current_type) {
    case Type::Boolean:
      this->data_boolean = other.data_boolean;
      break;
    case Type::Integer:
      this->data_integer = other.data_integer;
      break;
    case Type::Real:
      this->data_real = other.data_real;
      break;
    case Type::String:
      new (&this->data_string) String{std::move(other.data_string)};
      other.current_type = Type::Null;
      break;
    case Type::Array:
      new (&this->data_array) Array{std::move(other.data_array)};
      other.current_type = Type::Null;
      break;
    case Type::Object:
      new (&this->data_object) Object{std::move(other.data_object)};
      other.current_type = Type::Null;
      break;
    case Type::Decimal:
      this->data_decimal = other.data_decimal;
      other.data_decimal = nullptr;
      other.current_type = Type::Null;
      break;
    default:
      break;
  }
}

auto JSON::operator=(const JSON &other) -> JSON & {
  this->maybe_destruct_union();
  this->current_type = other.current_type;
  switch (other.current_type) {
    case Type::Boolean:
      this->data_boolean = other.data_boolean;
      break;
    case Type::Integer:
      this->data_integer = other.data_integer;
      break;
    case Type::Real:
      this->data_real = other.data_real;
      break;
    case Type::String:
      new (&this->data_string) String{other.data_string};
      break;
    case Type::Array:
      new (&this->data_array) Array{other.data_array};
      break;
    case Type::Object:
      new (&this->data_object) Object{other.data_object};
      break;
    case Type::Decimal:
      this->data_decimal = new Decimal{*other.data_decimal};
      break;
    default:
      break;
  }

  return *this;
}

auto JSON::operator=(JSON &&other) noexcept -> JSON & {
  this->maybe_destruct_union();
  this->current_type = other.current_type;
  switch (other.current_type) {
    case Type::Boolean:
      this->data_boolean = other.data_boolean;
      break;
    case Type::Integer:
      this->data_integer = other.data_integer;
      break;
    case Type::Real:
      this->data_real = other.data_real;
      break;
    case Type::String:
      new (&this->data_string) String{std::move(other.data_string)};
      other.current_type = Type::Null;
      break;
    case Type::Array:
      new (&this->data_array) Array{std::move(other.data_array)};
      other.current_type = Type::Null;
      break;
    case Type::Object:
      new (&this->data_object) Object{std::move(other.data_object)};
      other.current_type = Type::Null;
      break;
    case Type::Decimal:
      this->data_decimal = other.data_decimal;
      other.data_decimal = nullptr;
      other.current_type = Type::Null;
      break;
    default:
      break;
  }

  return *this;
}

JSON::~JSON() { this->maybe_destruct_union(); }

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

  if ((this->type() == Type::Decimal &&
       (other.type() == Type::Integer || other.type() == Type::Real)) ||
      ((this->type() == Type::Integer || this->type() == Type::Real) &&
       other.type() == Type::Decimal)) {
    const Decimal left = this->is_decimal()   ? this->to_decimal()
                         : this->is_integer() ? Decimal{this->to_integer()}
                                              : Decimal{this->to_real()};
    const Decimal right = other.is_decimal()   ? other.to_decimal()
                          : other.is_integer() ? Decimal{other.to_integer()}
                                               : Decimal{other.to_real()};
    return left < right;
  }

  if (this->type() != other.type()) {
    return this->current_type < other.current_type;
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
    case Type::Decimal:
      return this->to_decimal() < other.to_decimal();
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

  if ((this->type() == Type::Decimal &&
       (other.type() == Type::Integer || other.type() == Type::Real)) ||
      ((this->type() == Type::Integer || this->type() == Type::Real) &&
       other.type() == Type::Decimal)) {
    const Decimal left = this->is_decimal()   ? this->to_decimal()
                         : this->is_integer() ? Decimal{this->to_integer()}
                                              : Decimal{this->to_real()};
    const Decimal right = other.is_decimal()   ? other.to_decimal()
                          : other.is_integer() ? Decimal{other.to_integer()}
                                               : Decimal{other.to_real()};
    return left == right;
  }

  if (this->current_type != other.current_type) {
    return false;
  }

  switch (this->current_type) {
    case Type::Boolean:
      return this->data_boolean == other.data_boolean;
    case Type::Integer:
      return this->data_integer == other.data_integer;
    case Type::Real:
      return this->data_real == other.data_real;
    case Type::Decimal:
      return *this->data_decimal == *other.data_decimal;
    case Type::String:
      return this->data_string == other.data_string;
    case Type::Array:
      return this->data_array == other.data_array;
    case Type::Object:
      return this->data_object == other.data_object;
    default:
      return true;
  }
}

auto JSON::operator+(const JSON &other) const -> JSON {
  assert(this->is_number());
  assert(other.is_number());

  if (this->is_decimal() || other.is_decimal()) {
    const Decimal left = this->is_decimal()   ? this->to_decimal()
                         : this->is_integer() ? Decimal{this->to_integer()}
                                              : Decimal{this->to_real()};
    const Decimal right = other.is_decimal()   ? other.to_decimal()
                          : other.is_integer() ? Decimal{other.to_integer()}
                                               : Decimal{other.to_real()};
    return JSON{left + right};
  } else if (this->is_integer() && other.is_integer()) {
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

  if (this->is_decimal() || other.is_decimal()) {
    const Decimal left = this->is_decimal()   ? this->to_decimal()
                         : this->is_integer() ? Decimal{this->to_integer()}
                                              : Decimal{this->to_real()};
    const Decimal right = other.is_decimal()   ? other.to_decimal()
                          : other.is_integer() ? Decimal{other.to_integer()}
                                               : Decimal{other.to_real()};
    return JSON{left - right};
  } else if (this->is_integer() && other.is_integer()) {
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
  return this->current_type == Type::Boolean;
}

[[nodiscard]] auto JSON::is_null() const noexcept -> bool {
  return this->current_type == Type::Null;
}

[[nodiscard]] auto JSON::is_integer() const noexcept -> bool {
  return this->current_type == Type::Integer;
}

[[nodiscard]] auto JSON::is_real() const noexcept -> bool {
  return this->current_type == Type::Real;
}

[[nodiscard]] auto JSON::is_integral() const noexcept -> bool {
  switch (this->type()) {
    case Type::Integer:
      return true;
    case Type::Real: {
      Real integral = 0.0;
      return std::modf(this->to_real(), &integral) == 0.0;
    }
    case Type::Decimal:
      return this->to_decimal().is_integral();
    default:
      return false;
  }
}

[[nodiscard]] auto JSON::is_number() const noexcept -> bool {
  return this->is_integer() || this->is_real() || this->is_decimal();
}

[[nodiscard]] auto JSON::is_positive() const noexcept -> bool {
  switch (this->type()) {
    case Type::Integer:
      return this->to_integer() >= 0;
    case Type::Real:
      return this->to_real() >= static_cast<Real>(0.0);
    case Type::Decimal:
      return this->to_decimal() >= Decimal{0};
    default:
      return false;
  }
}

[[nodiscard]] auto JSON::is_string() const noexcept -> bool {
  return this->current_type == Type::String;
}

[[nodiscard]] auto JSON::is_array() const noexcept -> bool {
  return this->current_type == Type::Array;
}

[[nodiscard]] auto JSON::is_object() const noexcept -> bool {
  return this->current_type == Type::Object;
}

[[nodiscard]] auto JSON::is_decimal() const noexcept -> bool {
  return this->current_type == Type::Decimal;
}

[[nodiscard]] auto JSON::type() const noexcept -> Type {
  return this->current_type;
}

[[nodiscard]] auto JSON::to_boolean() const noexcept -> bool {
  assert(this->is_boolean());
  return this->data_boolean;
}

[[nodiscard]] auto JSON::to_integer() const noexcept -> Integer {
  assert(this->is_integer());
  return this->data_integer;
}

[[nodiscard]] auto JSON::to_real() const noexcept -> Real {
  assert(this->is_real());
  // This MUST not happen
  assert(!std::isinf(this->data_real));
  assert(!std::isnan(this->data_real));
  return this->data_real;
}

[[nodiscard]] auto JSON::to_decimal() const noexcept -> const Decimal & {
  assert(this->is_decimal());
  // This MUST not happen
  assert(this->data_decimal->is_finite());
  assert(!this->data_decimal->is_nan());
  return *this->data_decimal;
}

[[nodiscard]] auto JSON::to_string() const noexcept -> const JSON::String & {
  assert(this->is_string());
  return this->data_string;
}

[[nodiscard]] auto JSON::to_stringstream() const
    -> std::basic_istringstream<Char, CharTraits, Allocator<Char>> {
  return std::basic_istringstream<Char, CharTraits, Allocator<Char>>{
      this->data_string};
}

[[nodiscard]] auto JSON::as_array() const noexcept -> const JSON::Array & {
  assert(this->is_array());
  return this->data_array;
}

[[nodiscard]] auto JSON::as_array() noexcept -> JSON::Array & {
  assert(this->is_array());
  return this->data_array;
}

[[nodiscard]] auto JSON::as_object() noexcept -> Object & {
  assert(this->is_object());
  return this->data_object;
}

[[nodiscard]] auto JSON::as_object() const noexcept -> const Object & {
  assert(this->is_object());
  return this->data_object;
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
  return data_array.data.at(index);
}

[[nodiscard]] auto JSON::at(const typename JSON::Array::size_type index)
    -> JSON & {
  assert(this->is_array());
  assert(index < this->size());
  return this->data_array.data.at(index);
}

[[nodiscard]] auto JSON::at(const JSON::String &key) const -> const JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  const auto &object{this->data_object};
  return object.at(key, object.hash(key));
}

[[nodiscard]] auto JSON::at(const String &key,
                            const typename Object::hash_type hash) const
    -> const JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  return this->data_object.at(key, hash);
}

[[nodiscard]] auto JSON::at(const JSON::String &key) -> JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  auto &object{this->data_object};
  return object.at(key, object.hash(key));
}

[[nodiscard]] auto JSON::at(const String &key,
                            const typename Object::hash_type hash) -> JSON & {
  assert(this->is_object());
  assert(this->defines(key));
  return this->data_object.at(key, hash);
}

[[nodiscard]] auto JSON::at_or(const String &key,
                               const typename Object::hash_type hash,
                               const JSON &otherwise) const -> const JSON & {
  assert(this->is_object());
  const auto result{this->try_at(key, hash)};
  return result ? *result : otherwise;
}

[[nodiscard]] auto JSON::at_or(const String &key, const JSON &otherwise) const
    -> const JSON & {
  assert(this->is_object());
  return this->at_or(key, this->data_object.hash(key), otherwise);
}

[[nodiscard]] auto JSON::front() -> JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return this->data_array.data.front();
}

[[nodiscard]] auto JSON::front() const -> const JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return this->data_array.data.front();
}

[[nodiscard]] auto JSON::back() -> JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return this->data_array.data.back();
}

[[nodiscard]] auto JSON::back() const -> const JSON & {
  assert(this->is_array());
  assert(!this->empty());
  return this->data_array.data.back();
}

[[nodiscard]] auto JSON::size() const -> std::size_t {
  if (this->is_object()) {
    return this->object_size();
  } else if (this->is_array()) {
    return this->array_size();
  } else {
    return this->string_size();
  }
}

[[nodiscard]] auto JSON::string_size() const -> std::size_t {
  assert(this->is_string());
  return JSON::size(this->data_string);
}

[[nodiscard]] auto JSON::array_size() const -> std::size_t {
  assert(this->is_array());
  return this->data_array.data.size();
}

[[nodiscard]] auto JSON::object_size() const -> std::size_t {
  assert(this->is_object());
  return this->data_object.size();
}

[[nodiscard]] auto JSON::byte_size() const -> std::size_t {
  assert(this->is_string());
  return this->data_string.size();
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
    // The size of the union
    return 8;
  }
}

[[nodiscard]] auto JSON::fast_hash() const -> std::uint64_t {
  switch (this->current_type) {
    case Type::Null:
      return 2;
    case Type::Boolean:
      return this->to_boolean() ? 1 : 0;
    case Type::Integer:
      return 4 + (static_cast<std::uint64_t>(this->to_integer()) % 256);
    case Type::Real:
      return 5;
    case Type::String:
      return 3 + this->byte_size();
    case Type::Array:
      return std::accumulate(
          this->as_array().cbegin(), this->as_array().cend(),
          static_cast<std::uint64_t>(6),
          [](const std::uint64_t accumulator, const JSON &item) {
            return accumulator + 1 + item.fast_hash();
          });
    case Type::Object:
      return std::accumulate(this->as_object().cbegin(),
                             this->as_object().cend(),
                             static_cast<std::uint64_t>(7),
                             [](const std::uint64_t accumulator,
                                const typename Object::value_type &pair) {
                               return accumulator + 1 + pair.first.size() +
                                      pair.second.fast_hash();
                             });
    case Type::Decimal:
      return 8;
    default:
      assert(false);
      return 0;
  }
}

[[nodiscard]] auto JSON::divisible_by(const JSON &divisor) const -> bool {
  assert(this->is_number());
  assert(divisor.is_number());

  if (this->is_integer() && divisor.is_integer()) {
    const auto divisor_value{divisor.to_integer()};
    return divisor_value != 0 && this->to_integer() % divisor_value == 0;
  }

  if (!this->is_decimal() && !divisor.is_decimal()) {
    const auto divisor_value(divisor.as_real());
    if (divisor_value == 0.0) {
      return false;
    }

    const auto dividend_value{this->as_real()};

    // Every real number that represents an integral is divisible by 0.5.
    Real dividend_integral = 0;
    if (std::modf(dividend_value, &dividend_integral) == 0.0 &&
        divisor_value == 0.5) {
      return true;
    }

    const auto division{dividend_value / divisor_value};
    Real integral = 0;
    return !std::isinf(division) && !std::isnan(division) &&
           std::modf(division, &integral) == 0.0;
  }

  if (this->is_decimal() && divisor.is_decimal()) {
    return this->to_decimal().divisible_by(divisor.to_decimal());
  }

  if (this->is_decimal()) {
    if (divisor.is_integer()) {
      const Decimal divisor_decimal{divisor.to_integer()};
      return this->to_decimal().divisible_by(divisor_decimal);
    }

    const Decimal divisor_decimal{divisor.to_real()};
    return this->to_decimal().divisible_by(divisor_decimal);
  }

  if (this->is_integer()) {
    const Decimal dividend_decimal{this->to_integer()};
    return dividend_decimal.divisible_by(divisor.to_decimal());
  }

  const Decimal dividend_decimal{this->to_real()};
  return dividend_decimal.divisible_by(divisor.to_decimal());
}

[[nodiscard]] auto JSON::empty() const -> bool {
  if (this->is_object()) {
    return this->data_object.empty();
  } else if (this->is_array()) {
    return this->data_array.data.empty();
  } else {
    return this->data_string.empty();
  }
}

[[nodiscard]] auto JSON::try_at(const JSON::String &key) const -> const JSON * {
  assert(this->is_object());
  const auto &object{this->data_object};
  return object.try_at(key, object.hash(key));
}

[[nodiscard]] auto JSON::try_at(const String &key,
                                const typename Object::hash_type hash) const
    -> const JSON * {
  assert(this->is_object());
  const auto &object{this->data_object};
  return object.try_at(key, hash);
}

[[nodiscard]] auto JSON::defines(const JSON::String &key) const -> bool {
  assert(this->is_object());
  const auto &object{this->data_object};
  return object.defines(key, object.hash(key));
}

[[nodiscard]] auto
JSON::defines(const JSON::String &key,
              const typename JSON::Object::hash_type hash) const -> bool {
  assert(this->is_object());
  return this->data_object.defines(key, hash);
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

[[nodiscard]] auto JSON::contains(const JSON::String &input) const -> bool {
  assert(this->is_string());
  return this->to_string().find(input) != JSON::String::npos;
}

[[nodiscard]] auto JSON::contains(const JSON::String::value_type input) const
    -> bool {
  assert(this->is_string());
  return this->to_string().find(input) != JSON::String::npos;
}

[[nodiscard]] auto JSON::unique() const -> bool {
  assert(this->is_array());
  const auto &items{this->data_array.data};
  const auto size{items.size()};

  // Arrays of 0 or 1 item are unique by definition
  if (size <= 1) {
    return true;
  }

  // If we re-use the vector across threads, then we will segfault
  thread_local std::vector<std::uint64_t> cache;
  cache.clear();
  cache.resize(size);

  for (std::size_t index = 0; index < size; index++) {
    cache[index] = items[index].fast_hash();
  }

  for (std::size_t index = 0; index < size; index++) {
    for (std::size_t subindex = index + 1; subindex < size; subindex++) {
      if (cache[index] == cache[subindex] && items[index] == items[subindex]) {
        return false;
      }
    }
  }

  return true;
}

auto JSON::push_back(const JSON &value) -> void {
  assert(this->is_array());
  return this->data_array.data.push_back(value);
}

auto JSON::push_back(JSON &&value) -> void {
  assert(this->is_array());
  return this->data_array.data.push_back(std::move(value));
}

auto JSON::push_back_if_unique(const JSON &value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  assert(this->is_array());
  auto &array_data{this->as_array().data};
  const auto match{std::ranges::find(array_data, value)};
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
  const auto match{std::ranges::find(array_data, value)};
  if (match == array_data.cend()) {
    array_data.push_back(std::move(value));
    return {array_data.back(), true};
  } else {
    return {*match, false};
  }
}

auto JSON::assign(const JSON::String &key, const JSON &value) -> void {
  assert(this->is_object());
  this->data_object.emplace(key, value);
}

auto JSON::assign(const JSON::String &key, JSON &&value) -> void {
  assert(this->is_object());
  this->data_object.emplace(key, value);
}

auto JSON::try_assign_before(const String &key, const JSON &value,
                             const String &other) -> void {
  assert(this->is_object());
  this->data_object.try_emplace_before(key, value, other);
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

auto JSON::assign_assume_new(const JSON::String &key, JSON &&value) -> void {
  assert(this->is_object());
  this->data_object.emplace_assume_new(key, std::move(value));
}

auto JSON::assign_assume_new(JSON::String &&key, JSON &&value) -> void {
  assert(this->is_object());
  this->data_object.emplace_assume_new(std::move(key), std::move(value));
}

auto JSON::erase(const JSON::String &key) -> typename Object::size_type {
  assert(this->is_object());
  return this->data_object.erase(key);
}

auto JSON::erase_keys(std::initializer_list<JSON::String> keys) -> void {
  this->erase_keys(keys.begin(), keys.end());
}

auto JSON::erase(typename JSON::Array::const_iterator position) ->
    typename JSON::Array::iterator {
  assert(this->is_array());
  return this->data_array.data.erase(position);
}

auto JSON::erase(typename JSON::Array::const_iterator first,
                 typename JSON::Array::const_iterator last) ->
    typename JSON::Array::iterator {
  assert(this->is_array());
  return this->data_array.data.erase(first, last);
}

auto JSON::erase_if(const std::function<bool(const JSON &)> &predicate)
    -> void {
  assert(this->is_array());
  std::erase_if(this->data_array.data, predicate);
}

auto JSON::clear() -> void {
  if (this->is_object()) {
    this->data_object.clear();
  } else {
    this->data_array.data.clear();
  }
}

auto JSON::clear_except(std::initializer_list<JSON::String> keys) -> void {
  this->clear_except(keys.begin(), keys.end());
}

auto JSON::merge(const JSON::Object &other) -> void {
  assert(this->is_object());
  for (const auto &pair : other) {
    const auto maybe_key{this->try_at(pair.first, pair.hash)};
    if (maybe_key && maybe_key->is_object() && pair.second.is_object()) {
      this->at(pair.first, pair.hash).merge(pair.second.as_object());
    } else {
      this->assign(pair.first, pair.second);
    }
  }
}

[[nodiscard]] auto JSON::trim() const -> JSON::String {
  assert(this->is_string());
  auto copy = *this;
  copy.trim();
  return copy.to_string();
}

auto JSON::trim() -> const JSON::String & {
  assert(this->is_string());
  this->data_string.erase(this->data_string.find_last_not_of(TRIM_WHITESPACE) +
                          1);
  this->data_string.erase(0,
                          this->data_string.find_first_not_of(TRIM_WHITESPACE));
  return this->to_string();
}

[[nodiscard]] auto JSON::is_trimmed() const noexcept -> bool {
  assert(this->is_string());
  const auto &value{this->data_string};
  return value.empty() ||
         (value.find_first_of(TRIM_WHITESPACE) != 0 &&
          value.find_last_of(TRIM_WHITESPACE) != value.size() - 1);
}

auto JSON::reorder(const KeyComparison &compare) -> void {
  assert(this->is_object());
  this->data_object.reorder(compare);
}

auto JSON::rename(const JSON::String &key, JSON::String &&to) -> void {
  assert(this->is_object());
  auto &object{this->data_object};
  object.rename(key, object.hash(key), std::move(to), object.hash(to));
}

auto JSON::into(const JSON &other) -> void { this->operator=(other); }

auto JSON::into(JSON &&other) noexcept -> void {
  this->operator=(std::move(other));
}

auto JSON::into_array() -> void { this->into(JSON::make_array()); }

auto JSON::into_object() -> void { this->into(JSON::make_object()); }

auto JSON::maybe_destruct_union() -> void {
  switch (this->current_type) {
    case Type::String:
      this->data_string.~basic_string();
      break;
    case Type::Array:
      this->data_array.~JSONArray();
      break;
    case Type::Object:
      this->data_object.~JSONObject();
      break;
    case Type::Decimal:
      delete this->data_decimal;
      break;
    default:
      break;
  }

  this->current_type = Type::Null;
}

} // namespace sourcemeta::core
