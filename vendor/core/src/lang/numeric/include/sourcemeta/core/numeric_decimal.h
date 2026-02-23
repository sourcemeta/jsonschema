#ifndef SOURCEMETA_CORE_NUMERIC_DECIMAL_H_
#define SOURCEMETA_CORE_NUMERIC_DECIMAL_H_

#ifndef SOURCEMETA_CORE_NUMERIC_EXPORT
#include <sourcemeta/core/numeric_export.h>
#endif

#include <concepts> // std::integral
#include <cstdint>  // std::int32_t, std::int64_t, std::uint32_t, std::uint64_t
#include <string>   // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::is_signed_v

namespace sourcemeta::core {

/// @ingroup numeric
/// Represents an arbitrary-precision decimal number.
class SOURCEMETA_CORE_NUMERIC_EXPORT Decimal {
public:
  /// Construct a decimal number initialized to zero
  Decimal() noexcept;

  /// Destructor
  ~Decimal();

  /// Copy constructor
  Decimal(const Decimal &other);

  /// Move constructor
  Decimal(Decimal &&other) noexcept;

  /// Construct a decimal number from a signed integral type
  template <typename T>
    requires std::integral<T> && std::is_signed_v<T> &&
             (!std::same_as<T, std::int64_t>)
  Decimal(const T value) : Decimal{static_cast<std::int64_t>(value)} {}

  /// Construct a decimal number from a 64-bit signed integer
  Decimal(std::int64_t value);

  /// Construct a decimal number from an unsigned integral type
  template <typename T>
    requires std::integral<T> && std::is_unsigned_v<T> &&
             (!std::same_as<T, std::uint64_t>)
  Decimal(const T value) : Decimal{static_cast<std::uint64_t>(value)} {}

  /// Construct a decimal number from a 64-bit unsigned integer
  Decimal(std::uint64_t value);

  /// Construct a decimal number from a 32-bit float
  explicit Decimal(float value);

  /// Construct a decimal number from a 64-bit double
  explicit Decimal(double value);

  /// Construct a decimal number from a C-string
  explicit Decimal(const char *const value);

  /// Construct a decimal number from a C++ string
  explicit Decimal(const std::string &value);

  /// Construct a decimal number from a string view
  explicit Decimal(const std::string_view value);

  /// Copy assignment operator
  auto operator=(const Decimal &other) -> Decimal &;

  /// Move assignment operator
  auto operator=(Decimal &&other) noexcept -> Decimal &;

  /// Create a quiet NaN (Not a Number) value with an optional payload
  [[nodiscard]] static auto nan(std::uint64_t payload = 0) -> Decimal;

  /// Create a signaling NaN value with an optional payload
  [[nodiscard]] static auto snan(std::uint64_t payload = 0) -> Decimal;

  /// Create a positive infinity value
  [[nodiscard]] static auto infinity() -> Decimal;

  /// Create a negative infinity value
  [[nodiscard]] static auto negative_infinity() -> Decimal;

  /// Convert the decimal number to scientific notation string
  [[nodiscard]] auto to_scientific_string() const -> std::string;

  /// Convert the decimal number to a plain string representation
  [[nodiscard]] auto to_string() const -> std::string;

  /// Convert the decimal number to a 64-bit signed integer
  [[nodiscard]] auto to_int64() const -> std::int64_t;

  /// Convert the decimal number to a 32-bit signed integer
  [[nodiscard]] auto to_int32() const -> std::int32_t;

  /// Convert the decimal number to a 64-bit unsigned integer
  [[nodiscard]] auto to_uint64() const -> std::uint64_t;

  /// Convert the decimal number to a 32-bit unsigned integer
  [[nodiscard]] auto to_uint32() const -> std::uint32_t;

  /// Convert the decimal number to a 32-bit float
  [[nodiscard]] auto to_float() const -> float;

  /// Convert the decimal number to a 64-bit double
  [[nodiscard]] auto to_double() const -> double;

  /// Check if the decimal number is zero
  [[nodiscard]] auto is_zero() const -> bool;

  /// Check if the decimal number represents an integer value, which includes a
  /// number like `3.0`
  [[nodiscard]] auto is_integral() const -> bool;

  /// Check if the decimal number represents an integer value _without_ a
  /// decimal component in its original representation.
  [[nodiscard]] auto is_integer() const -> bool;

  /// Check if the decimal number is finite
  [[nodiscard]] auto is_finite() const -> bool;

  /// Check if the decimal number is a real number (finite and not NaN)
  [[nodiscard]] auto is_real() const -> bool;

  /// Check if the decimal number can be represented as a 32-bit float without
  /// precision loss
  [[nodiscard]] auto is_float() const -> bool;

  /// Check if the decimal number can be represented as a 64-bit double without
  /// precision loss
  [[nodiscard]] auto is_double() const -> bool;

  /// Check if the decimal number fits in a 32-bit signed integer
  [[nodiscard]] auto is_int32() const -> bool;

  /// Check if the decimal number fits in a 64-bit signed integer
  [[nodiscard]] auto is_int64() const -> bool;

  /// Check if the decimal number fits in a 32-bit unsigned integer
  [[nodiscard]] auto is_uint32() const -> bool;

  /// Check if the decimal number fits in a 64-bit unsigned integer
  [[nodiscard]] auto is_uint64() const -> bool;

  /// Check if the decimal number is NaN (Not a Number), either quiet or
  /// signaling
  [[nodiscard]] auto is_nan() const -> bool;

  /// Check if the decimal number is a signaling NaN
  [[nodiscard]] auto is_snan() const -> bool;

  /// Check if the decimal number is a quiet NaN
  [[nodiscard]] auto is_qnan() const -> bool;

  /// Get the payload of a NaN value (0 if no payload)
  [[nodiscard]] auto nan_payload() const -> std::uint64_t;

  /// Check if the decimal number is infinite
  [[nodiscard]] auto is_infinite() const -> bool;

  /// Check if the decimal number is signed (negative, including -0)
  [[nodiscard]] auto is_signed() const -> bool;

  /// Round the decimal number to an integral value
  [[nodiscard]] auto to_integral() const -> Decimal;

  /// Check if this decimal number is divisible by another
  [[nodiscard]] auto divisible_by(const Decimal &divisor) const -> bool;

  /// Strip trailing zeros from the coefficient
  [[nodiscard]] auto reduce() const -> Decimal;

  /// Return the adjusted exponent (floor of base-10 logarithm)
  [[nodiscard]] auto logb() const -> Decimal;

  /// Scale the number by a power of 10
  [[nodiscard]] auto scale_by(const Decimal &scale) const -> Decimal;

  /// Check if two numbers have the same quantum (exponent)
  [[nodiscard]] auto same_quantum(const Decimal &other) const -> bool;

  /// IEEE 754 total ordering comparison returning -1, 0, or 1
  [[nodiscard]] auto compare_total(const Decimal &other) const -> Decimal;

  /// Integer division (truncate toward zero)
  [[nodiscard]] auto divide_integer(const Decimal &other) const -> Decimal;

  /// Add another decimal number to this one
  auto operator+=(const Decimal &other) -> Decimal &;

  /// Subtract another decimal number from this one
  auto operator-=(const Decimal &other) -> Decimal &;

  /// Multiply this decimal number by another
  auto operator*=(const Decimal &other) -> Decimal &;

  /// Divide this decimal number by another
  auto operator/=(const Decimal &other) -> Decimal &;

  /// Compute the modulo of this decimal number with another
  auto operator%=(const Decimal &other) -> Decimal &;

  /// Add two decimal numbers
  [[nodiscard]] auto operator+(const Decimal &other) const -> Decimal;

  /// Subtract two decimal numbers
  [[nodiscard]] auto operator-(const Decimal &other) const -> Decimal;

  /// Multiply two decimal numbers
  [[nodiscard]] auto operator*(const Decimal &other) const -> Decimal;

  /// Divide two decimal numbers
  [[nodiscard]] auto operator/(const Decimal &other) const -> Decimal;

  /// Compute the modulo of two decimal numbers
  [[nodiscard]] auto operator%(const Decimal &other) const -> Decimal;

  /// Unary negation operator
  [[nodiscard]] auto operator-() const -> Decimal;

  /// Unary plus operator
  [[nodiscard]] auto operator+() const -> Decimal;

  /// Prefix increment operator
  auto operator++() -> Decimal &;

  /// Postfix increment operator
  auto operator++(int) -> Decimal;

  /// Prefix decrement operator
  auto operator--() -> Decimal &;

  /// Postfix decrement operator
  auto operator--(int) -> Decimal;

  /// Check if two decimal numbers are equal
  [[nodiscard]] auto operator==(const Decimal &other) const -> bool;

  /// Check if two decimal numbers are not equal
  [[nodiscard]] auto operator!=(const Decimal &other) const -> bool;

  /// Check if this decimal number is less than another
  [[nodiscard]] auto operator<(const Decimal &other) const -> bool;

  /// Check if this decimal number is less than or equal to another
  [[nodiscard]] auto operator<=(const Decimal &other) const -> bool;

  /// Check if this decimal number is greater than another
  [[nodiscard]] auto operator>(const Decimal &other) const -> bool;

  /// Check if this decimal number is greater than or equal to another
  [[nodiscard]] auto operator>=(const Decimal &other) const -> bool;

private:
  std::int64_t coefficient_{0};
  std::uint64_t coefficient_high_{0};
  std::int32_t exponent_{0};
  std::uint8_t flags_{0};
};

template <typename T>
  requires std::integral<T>
inline auto operator+(const T left, const Decimal &right) -> Decimal {
  return Decimal{left} + right;
}

template <typename T>
  requires std::integral<T>
inline auto operator-(const T left, const Decimal &right) -> Decimal {
  return Decimal{left} - right;
}

template <typename T>
  requires std::integral<T>
inline auto operator*(const T left, const Decimal &right) -> Decimal {
  return Decimal{left} * right;
}

template <typename T>
  requires std::integral<T>
inline auto operator/(const T left, const Decimal &right) -> Decimal {
  return Decimal{left} / right;
}

template <typename T>
  requires std::integral<T>
inline auto operator%(const T left, const Decimal &right) -> Decimal {
  return Decimal{left} % right;
}

template <typename T>
  requires std::integral<T>
inline auto operator==(const T left, const Decimal &right) -> bool {
  return Decimal{left} == right;
}

template <typename T>
  requires std::integral<T>
inline auto operator!=(const T left, const Decimal &right) -> bool {
  return Decimal{left} != right;
}

template <typename T>
  requires std::integral<T>
inline auto operator<(const T left, const Decimal &right) -> bool {
  return Decimal{left} < right;
}

template <typename T>
  requires std::integral<T>
inline auto operator<=(const T left, const Decimal &right) -> bool {
  return Decimal{left} <= right;
}

template <typename T>
  requires std::integral<T>
inline auto operator>(const T left, const Decimal &right) -> bool {
  return Decimal{left} > right;
}

template <typename T>
  requires std::integral<T>
inline auto operator>=(const T left, const Decimal &right) -> bool {
  return Decimal{left} >= right;
}

} // namespace sourcemeta::core

#endif
