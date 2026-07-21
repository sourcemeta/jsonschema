#ifndef SOURCEMETA_CORE_JSONLD_SERIALISE_H_
#define SOURCEMETA_CORE_JSONLD_SERIALISE_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/numeric.h>

#include <algorithm> // std::copy
#include <array>     // std::array
#include <cassert>   // assert
#include <charconv>  // std::to_chars, std::from_chars, std::chars_format
#include <cstddef>   // std::size_t
#include <cstdint>   // std::int32_t
#include <optional>  // std::optional, std::nullopt
#include <utility>   // std::unreachable

// Serialisers that turn a native number or boolean into the string lexical
// form of an RDF literal when a position carries an explicit datatype. Plain
// JSON stringification is not enough for two reasons. It picks the shortest
// round-trip spelling in whichever notation is shorter, so magnitudes like
// 0.00001 or 1e21 come out in scientific notation, which is outside the
// lexical space of exponent-free datatypes like xsd:decimal (XSD 1.1 Part 2
// Section 3.3.3). And for xsd:double and xsd:float the output must be the
// exact canonical form that conforming RDF conversion derives from native
// numbers (JSON-LD 1.1 API Section 8.6), so that the literal terms consumers
// obtain stay byte-identical to the native behaviour they get today

namespace sourcemeta::core {

inline constexpr JSON::StringView DATATYPE_XSD_DOUBLE{
    "http://www.w3.org/2001/XMLSchema#double"};
inline constexpr JSON::StringView DATATYPE_XSD_FLOAT{
    "http://www.w3.org/2001/XMLSchema#float"};

// Both floating point datatypes take the scientific canonical form
inline auto is_floating_point_datatype(const JSON::StringView datatype)
    -> bool {
  return datatype == DATATYPE_XSD_DOUBLE || datatype == DATATYPE_XSD_FLOAT;
}

// The double form of JSON-LD 1.1 API Section 8.6, where the mantissa is
// "rounded to 15 digits after the decimal point" with trailing zeros dropped
// down to a single digit after the required decimal point, the exponent
// carries no plus sign or leading zeros, and "the canonical representation
// for zero is 0.0E0". The form is assembled in the conversion buffer itself
// so the function performs at most a single allocation
inline auto scientific_lexical_form(const double value) -> JSON::String {
  if (value == 0.0) {
    return "0.0E0";
  }

  std::array<char, 32> buffer{};
  const auto conversion{std::to_chars(buffer.data(),
                                      buffer.data() + buffer.size(), value,
                                      std::chars_format::scientific, 15)};
  assert(conversion.ec == std::errc{});
  const JSON::StringView digits{
      buffer.data(), static_cast<std::size_t>(conversion.ptr - buffer.data())};
  const auto exponent_marker{digits.find('e')};
  assert(exponent_marker != JSON::StringView::npos);

  // The exponent characters are copied out first, as the assembled form is
  // written over the region they occupy
  auto exponent{digits.substr(exponent_marker + 1)};
  const bool negative_exponent{exponent.front() == '-'};
  if (negative_exponent || exponent.front() == '+') {
    exponent.remove_prefix(1);
  }
  const auto first_significant_exponent{exponent.find_first_not_of('0')};
  std::array<char, 8> exponent_digits{};
  std::size_t exponent_size{0};
  if (first_significant_exponent == JSON::StringView::npos) {
    exponent_digits[exponent_size] = '0';
    exponent_size += 1;
  } else {
    for (const auto character : exponent.substr(first_significant_exponent)) {
      exponent_digits[exponent_size] = character;
      exponent_size += 1;
    }
  }

  const auto mantissa{digits.substr(0, exponent_marker)};
  const auto last_significant{mantissa.find_last_not_of('0')};
  assert(last_significant != JSON::StringView::npos);
  const auto mantissa_size{mantissa.at(last_significant) == '.'
                               ? last_significant + 2
                               : last_significant + 1};
  auto *cursor{buffer.data() + mantissa_size};
  *cursor = 'E';
  cursor += 1;
  if (negative_exponent) {
    *cursor = '-';
    cursor += 1;
  }
  cursor = std::copy(exponent_digits.data(),
                     exponent_digits.data() + exponent_size, cursor);
  return {buffer.data(), static_cast<std::size_t>(cursor - buffer.data())};
}

// The shortest decimal expansion that parses back to the same value, with no
// exponent so that it stays within the lexical space of datatypes that do not
// admit scientific notation
inline auto fixed_lexical_form(const double value) -> JSON::String {
  std::array<char, 344> buffer{};
  const auto conversion{std::to_chars(buffer.data(),
                                      buffer.data() + buffer.size(), value,
                                      std::chars_format::fixed)};
  assert(conversion.ec == std::errc{});
  return {buffer.data(),
          static_cast<std::size_t>(conversion.ptr - buffer.data())};
}

// The most extreme decimal exponent that still expands to an exponent-free
// spelling. A tiny exponent notation input would otherwise demand an
// expansion of up to two billion digits, so magnitudes beyond this bound
// produce no result and the native value passes through untouched
inline constexpr std::int64_t MAXIMUM_EXPANSION_EXPONENT{10000};

// The exact exponent-free expansion of an arbitrary precision decimal, with
// trailing zeros stripped so that the spelling is minimal. The expansion is
// assembled once into a preallocated result out of views over the scientific
// form
inline auto fixed_lexical_form(const Decimal &value)
    -> std::optional<JSON::String> {
  assert(value.is_finite());
  if (!value.is_zero()) {
    const auto magnitude{value.logb().to_int64()};
    if (magnitude > MAXIMUM_EXPANSION_EXPONENT ||
        magnitude < -MAXIMUM_EXPANSION_EXPONENT) {
      return std::nullopt;
    }
  }

  const auto scientific{value.reduce().to_scientific_string()};
  const JSON::StringView view{scientific};
  const auto exponent_marker{view.find('e')};
  assert(exponent_marker != JSON::StringView::npos);
  auto mantissa{view.substr(0, exponent_marker)};
  const bool negative{mantissa.front() == '-'};
  if (negative) {
    mantissa.remove_prefix(1);
  }
  const auto point{mantissa.find('.')};
  const auto head{mantissa.substr(
      0, point == JSON::StringView::npos ? mantissa.size() : point)};
  const auto tail{point == JSON::StringView::npos ? JSON::StringView{}
                                                  : mantissa.substr(point + 1)};
  assert(head.size() == 1);

  auto exponent{view.substr(exponent_marker + 1)};
  if (exponent.front() == '+') {
    exponent.remove_prefix(1);
  }
  std::int32_t adjusted{0};
  [[maybe_unused]] const auto conversion{std::from_chars(
      exponent.data(), exponent.data() + exponent.size(), adjusted)};
  assert(conversion.ec == std::errc{});

  const auto count{static_cast<std::int32_t>(head.size() + tail.size())};
  const auto sign_size{static_cast<std::size_t>(negative ? 1 : 0)};
  JSON::String result;
  if (adjusted >= count - 1) {
    result.reserve(sign_size + static_cast<std::size_t>(adjusted) + 1);
    if (negative) {
      result += '-';
    }
    result += head;
    result += tail;
    result.append(static_cast<std::size_t>(adjusted - (count - 1)), '0');
  } else if (adjusted >= 0) {
    result.reserve(sign_size + static_cast<std::size_t>(count) + 1);
    if (negative) {
      result += '-';
    }
    result += head;
    result += tail.substr(0, static_cast<std::size_t>(adjusted));
    result += '.';
    result += tail.substr(static_cast<std::size_t>(adjusted));
  } else {
    const auto padding{static_cast<std::size_t>(-adjusted) - 1};
    result.reserve(sign_size + 2 + padding + static_cast<std::size_t>(count));
    if (negative) {
      result += '-';
    }
    result += "0.";
    result.append(padding, '0');
    result += head;
    result += tail;
  }
  return result;
}

// A native number or boolean paired with an explicit datatype becomes its
// canonical string lexical form, as no result is produced for other values.
// JSON-LD 1.1 API Section 8.2.2 converts a native number with a non-zero
// fractional part to the "canonical lexical form of an xsd:double" no matter
// which datatype was requested, yielding ill-typed literals for datatypes
// whose lexical space has no exponent, whereas a string value passes through
// RDF conversion verbatim
inline auto typed_literal_lexical_form(const JSON &value,
                                       const JSON::StringView datatype)
    -> std::optional<JSON::String> {
  switch (value.type()) {
    case JSON::Type::Boolean:
      return JSON::String{value.to_boolean() ? "true" : "false"};
    case JSON::Type::Integer: {
      if (is_floating_point_datatype(datatype)) {
        return scientific_lexical_form(static_cast<double>(value.to_integer()));
      }
      std::array<char, 20> buffer{};
      const auto conversion{std::to_chars(
          buffer.data(), buffer.data() + buffer.size(), value.to_integer())};
      assert(conversion.ec == std::errc{});
      return JSON::String{buffer.data(), static_cast<std::size_t>(
                                             conversion.ptr - buffer.data())};
    }
    case JSON::Type::Real:
      return is_floating_point_datatype(datatype)
                 ? scientific_lexical_form(value.to_real())
                 : fixed_lexical_form(value.to_real());
    case JSON::Type::Decimal: {
      const auto &decimal{value.to_decimal()};
      if (is_floating_point_datatype(datatype)) {
        const auto converted{to_double(decimal.to_scientific_string())};
        if (converted.has_value()) {
          return scientific_lexical_form(converted.value());
        }
        // A magnitude outside the double range has no scientific canonical
        // form to borrow, so the exact expansion is the deterministic
        // fallback
      }
      return fixed_lexical_form(decimal);
    }
    case JSON::Type::Null:
    case JSON::Type::String:
    case JSON::Type::Array:
    case JSON::Type::Object:
      return std::nullopt;
  }

  std::unreachable();
}

} // namespace sourcemeta::core

#endif
