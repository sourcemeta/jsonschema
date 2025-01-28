#ifndef SOURCEMETA_JSONBINPACK_RUNTIME_ENCODING_H_
#define SOURCEMETA_JSONBINPACK_RUNTIME_ENCODING_H_

#include <sourcemeta/jsonbinpack/numeric.h>

#include <sourcemeta/core/json.h>

#include <cstdint> // std::int64_t, std::uint64_t
#include <memory>  // std::shared_ptr
#include <variant> // std::variant
#include <vector>  // std::vector

namespace sourcemeta::jsonbinpack {

// Forward declarations for the sole purpose of being bale to define circular
// structures
#ifndef DOXYGEN
struct BOUNDED_MULTIPLE_8BITS_ENUM_FIXED;
struct FLOOR_MULTIPLE_ENUM_VARINT;
struct ROOF_MULTIPLE_MIRROR_ENUM_VARINT;
struct ARBITRARY_MULTIPLE_ZIGZAG_VARINT;
struct DOUBLE_VARINT_TUPLE;
struct BYTE_CHOICE_INDEX;
struct LARGE_CHOICE_INDEX;
struct TOP_LEVEL_BYTE_CHOICE_INDEX;
struct CONST_NONE;
struct ANY_PACKED_TYPE_TAG_BYTE_PREFIX;
struct UTF8_STRING_NO_LENGTH;
struct FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED;
struct ROOF_VARINT_PREFIX_UTF8_STRING_SHARED;
struct BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED;
struct RFC3339_DATE_INTEGER_TRIPLET;
struct PREFIX_VARINT_LENGTH_STRING_SHARED;
struct FIXED_TYPED_ARRAY;
struct BOUNDED_8BITS_TYPED_ARRAY;
struct FLOOR_TYPED_ARRAY;
struct ROOF_TYPED_ARRAY;
struct FIXED_TYPED_ARBITRARY_OBJECT;
struct VARINT_TYPED_ARBITRARY_OBJECT;
#endif

/// @ingroup runtime
/// Represents an encoding
using Encoding = std::variant<
    BOUNDED_MULTIPLE_8BITS_ENUM_FIXED, FLOOR_MULTIPLE_ENUM_VARINT,
    ROOF_MULTIPLE_MIRROR_ENUM_VARINT, ARBITRARY_MULTIPLE_ZIGZAG_VARINT,
    DOUBLE_VARINT_TUPLE, BYTE_CHOICE_INDEX, LARGE_CHOICE_INDEX,
    TOP_LEVEL_BYTE_CHOICE_INDEX, CONST_NONE, ANY_PACKED_TYPE_TAG_BYTE_PREFIX,
    UTF8_STRING_NO_LENGTH, FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED,
    ROOF_VARINT_PREFIX_UTF8_STRING_SHARED,
    BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED, RFC3339_DATE_INTEGER_TRIPLET,
    PREFIX_VARINT_LENGTH_STRING_SHARED, FIXED_TYPED_ARRAY,
    BOUNDED_8BITS_TYPED_ARRAY, FLOOR_TYPED_ARRAY, ROOF_TYPED_ARRAY,
    FIXED_TYPED_ARBITRARY_OBJECT, VARINT_TYPED_ARBITRARY_OBJECT>;

/// @ingroup runtime
/// @defgroup encoding_integer Integer Encodings
/// @{

// clang-format off
/// @brief The encoding consists of the integer value divided by the
/// `multiplier`, minus the ceil of `minimum` divided by the `multiplier`,
/// encoded as an 8-bit fixed-length unsigned integer.
///
/// ### Options
///
/// | Option       | Type   | Description                 |
/// |--------------|--------|-----------------------------|
/// | `minimum`    | `int`  | The inclusive minimum value |
/// | `maximum`    | `int`  | The inclusive maximum value |
/// | `multiplier` | `uint` | The multiplier value        |
///
/// ### Conditions
///
/// | Condition                    | Description                                                         |
/// |------------------------------|---------------------------------------------------------------------|
/// | `value >= minimum`           | The input value must be greater than or equal to the minimum        |
/// | `value <= maximum`           | The input value must be less than or equal to the maximum           |
/// | `value % multiplier == 0`    | The input value must be divisible by the multiplier                 |
/// | `floor(maximum / multiplier) - ceil(minimum / multiplier) < 2 ** 8` | The divided range must be representable in 8 bits |
///
/// ### Examples
///
/// Given the input value 15, where the minimum is 1, the maximum is 19, and the
/// multiplier is 5, the encoding results in the 8-bit unsigned integer 2:
///
/// ```
/// +------+
/// | 0x02 |
/// +------+
/// ```
// clang-format on
struct BOUNDED_MULTIPLE_8BITS_ENUM_FIXED {
  /// The inclusive minimum value
  const std::int64_t minimum;
  /// The inclusive maximum value
  const std::int64_t maximum;
  /// The multiplier value
  const std::uint64_t multiplier;
};

// clang-format off
/// @brief The encoding consists of the integer value divided by the
/// `multiplier`, minus the ceil of `minimum` divided by the `multiplier`,
/// encoded as a Base-128 64-bit Little Endian variable-length unsigned
/// integer.
///
/// ### Options
///
/// | Option       | Type   | Description                 |
/// |--------------|--------|-----------------------------|
/// | `minimum`    | `int`  | The inclusive minimum value |
/// | `multiplier` | `uint` | The multiplier value        |
///
/// ### Conditions
///
/// | Condition                 | Description                                                  |
/// |---------------------------|--------------------------------------------------------------|
/// | `value >= minimum`        | The input value must be greater than or equal to the minimum |
/// | `value % multiplier == 0` | The input value must be divisible by the multiplier          |
///
/// ### Examples
///
/// Given the input value 1000, where the minimum is -2 and the multiplier is 4,
/// the encoding results in the Base-128 64-bit Little Endian variable-length
/// unsigned integer 250:
///
/// ```
/// +------+------+
/// | 0xfa | 0x01 |
/// +------+------+
/// ```
// clang-format on
struct FLOOR_MULTIPLE_ENUM_VARINT {
  /// The inclusive minimum value
  const std::int64_t minimum;
  /// The multiplier value
  const std::uint64_t multiplier;
};

// clang-format off
/// @brief The encoding consists of the floor of `maximum` divided by the
/// `multiplier`, minus the integer value divided by the `multiplier`, encoded
/// as a Base-128 64-bit Little Endian variable-length unsigned integer.
///
/// ### Options
///
/// | Option       | Type   | Description                 |
/// |--------------|--------|-----------------------------|
/// | `maximum`    | `int`  | The inclusive maximum value |
/// | `multiplier` | `uint` | The multiplier value        |
///
/// ### Conditions
///
/// | Condition                 | Description                                               |
/// |---------------------------|-----------------------------------------------------------|
/// | `value <= maximum`        | The input value must be less than or equal to the maximum |
/// | `value % multiplier == 0` | The input value must be divisible by the multiplier       |
///
/// ### Examples
///
/// Given the input value 5, where the maximum is 16 and the multiplier is 5, the
/// encoding results in the Base-128 64-bit Little Endian variable-length unsigned
/// integer 2:
///
/// ```
/// +------+
/// | 0x02 |
/// +------+
/// ```
// clang-format on
struct ROOF_MULTIPLE_MIRROR_ENUM_VARINT {
  /// The inclusive maximum value
  const std::int64_t maximum;
  /// The multiplier value
  const std::uint64_t multiplier;
};

// clang-format off
/// @brief The encoding consists of the the integer value divided by the
/// `multiplier` encoded as a ZigZag-encoded Base-128 64-bit Little Endian
/// variable-length unsigned integer.
///
/// ### Options
///
/// | Option       | Type   | Description          |
/// |--------------|--------|----------------------|
/// | `multiplier` | `uint` | The multiplier value |
///
/// ### Conditions
///
/// | Condition                 | Description                                         |
/// |---------------------------|-----------------------------------------------------|
/// | `value % multiplier == 0` | The input value must be divisible by the multiplier |
///
/// ### Examples
///
/// Given the input value 10, where the multiplier is 5, the encoding results in
/// the Base-128 64-bit Little Endian variable-length unsigned integer 4:
///
/// ```
/// +------+
/// | 0x04 |
/// +------+
/// ```
// clang-format on
struct ARBITRARY_MULTIPLE_ZIGZAG_VARINT {
  /// The multiplier value
  const std::uint64_t multiplier;
};

/// @}

/// @ingroup runtime
/// @defgroup encoding_number Number Encodings
/// @{

// clang-format off
/// @brief The encoding consists of a sequence of two integers: The signed
/// integer that results from concatenating the integral part and the decimal
/// part of the number, if any, as a ZigZag-encoded Base-128 64-bit Little
/// Endian variable-length unsigned integer; and the position of the decimal
/// mark from the last digit of the number encoded as a Base-128 64-bit Little
/// Endian variable-length unsigned integer.
///
/// ### Options
///
/// None
///
/// ### Conditions
///
/// None
///
/// ### Examples
///
/// Given the input value 3.14, the encoding results in the variable-length integer
/// 628 (the ZigZag encoding of 314) followed by the variable-length unsigned
/// integer 2 (the number of decimal digits in the number).
///
/// ```
/// +------+------+------+
/// | 0xf4 | 0x04 | 0x02 |
/// +------+------+------+
/// ```
///
/// Real numbers that represent integers are encoded with a decimal mark of zero.
/// Given the input value -5.0, the encoding results in the variable-length integer
/// 9 (the ZigZag encoding of -5) followed by the variable-length unsigned integer
/// 0.
///
/// ```
/// +------+------+
/// | 0x09 | 0x00 |
/// +------+------+
/// ```
// clang-format on
struct DOUBLE_VARINT_TUPLE {};

/// @}

/// @ingroup runtime
/// @defgroup encoding_any Any Encodings
/// @{

// clang-format off
/// @brief The encoding consists of an index to the enumeration choices encoded
/// as an 8-bit fixed-length unsigned integer.
///
/// ### Options
///
/// | Option    | Type    | Description              |
/// |-----------|---------|--------------------------|
/// | `choices` | `any[]` | The set of choice values |
///
/// ### Conditions
///
/// | Condition                | Description                                            |
/// |--------------------------|--------------------------------------------------------|
/// | `len(choices) > 0`       | The choices array must not be empty                    |
/// | `len(choices) <  2 ** 8` | The number of choices must be representable in 8 bits  |
/// | `value in choices`       | The input value must be included in the set of choices |
///
/// ### Examples
///
/// Given an enumeration `[ "foo", "bar", "baz" ]` and an input value `"bar"`, the
/// encoding results in the unsigned 8 bit integer 1:
///
/// ```
/// +------+
/// | 0x01 |
/// +------+
/// ```
///
/// Given an enumeration `[ "foo", "bar", "baz" ]` and an input value `"foo"`, the
/// encoding results in the unsigned 8 bit integer 0:
///
/// ```
/// +------+
/// | 0x00 |
/// +------+
/// ```
// clang-format on
struct BYTE_CHOICE_INDEX {
  /// The set of choice values
  const std::vector<sourcemeta::core::JSON> choices;
};

// clang-format off
/// @brief The encoding consists of an index to the enumeration choices encoded
/// as a Base-128 64-bit Little Endian variable-length unsigned integer.
///
/// ### Options
///
/// | Option    | Type    | Description              |
/// |-----------|---------|--------------------------|
/// | `choices` | `any[]` | The set of choice values |
///
/// ### Conditions
///
/// | Condition                | Description                                            |
/// |--------------------------|--------------------------------------------------------|
/// | `len(choices) > 0`       | The choices array must not be empty                    |
/// | `value in choices`       | The input value must be included in the set of choices |
///
/// ### Examples
///
/// Given an enumeration with 1000 members and an input value that equals the 300th
/// enumeration value, the encoding results in the Base-128 64-bit Little Endian
/// variable-length unsigned integer 300:
///
/// ```
/// +------+------+
/// | 0xac | 0x02 |
/// +------+------+
/// ```
// clang-format on
struct LARGE_CHOICE_INDEX {
  /// The set of choice values
  const std::vector<sourcemeta::core::JSON> choices;
};

// clang-format off
/// @brief If the input value corresponds to the index 0 to the enumeration
/// choices, the encoding stores no data. Otherwise, the encoding consists of
/// an index to the enumeration choices minus 1 encoded as an 8-bit
/// fixed-length unsigned integer.
///
/// ### Options
///
/// | Option    | Type    | Description              |
/// |-----------|---------|--------------------------|
/// | `choices` | `any[]` | The set of choice values |
///
/// ### Conditions
///
/// | Condition                | Description                                            |
/// |--------------------------|--------------------------------------------------------|
/// | `len(choices) > 0`       | The choices array must not be empty                    |
/// | `len(choices) <  2 ** 8` | The number of choices must be representable in 8 bits  |
/// | `value in choices`       | The input value must be included in the set of choices |
///
/// ### Examples
///
/// Given an enumeration `[ "foo", "bar", "baz" ]` and an input value `"bar"`, the
/// encoding results in the unsigned 8 bit integer 0:
///
/// ```
/// +------+
/// | 0x00 |
/// +------+
/// ```
///
/// Given an enumeration `[ "foo", "bar", "baz" ]` and an input value `"foo"`, the
/// value is not encoded.
// clang-format on
struct TOP_LEVEL_BYTE_CHOICE_INDEX {
  /// The set of choice values
  const std::vector<sourcemeta::core::JSON> choices;
};

// clang-format off
/// @brief The constant input value is not encoded.
///
/// ### Options
///
/// | Option  | Type  | Description        |
/// |---------|-------|--------------------|
/// | `value` | `any` | The constant value |
///
/// ### Conditions
///
/// None
///
/// ### Examples
///
/// The input value that matches the `value` option is not encoded.
// clang-format on
struct CONST_NONE {
  /// The constant value
  const sourcemeta::core::JSON value;
};

// TODO: Write brief description
struct ANY_PACKED_TYPE_TAG_BYTE_PREFIX {};
#ifndef DOXYGEN
namespace internal::ANY_PACKED_TYPE_TAG_BYTE_PREFIX {
constexpr auto type_size = 3;
constexpr std::uint8_t TYPE_SHARED_STRING = 0b00000000;
constexpr std::uint8_t TYPE_STRING = 0b00000001;
constexpr std::uint8_t TYPE_LONG_STRING = 0b00000010;
constexpr std::uint8_t TYPE_OBJECT = 0b00000011;
constexpr std::uint8_t TYPE_ARRAY = 0b00000100;
constexpr std::uint8_t TYPE_POSITIVE_INTEGER_BYTE = 0b00000101;
constexpr std::uint8_t TYPE_NEGATIVE_INTEGER_BYTE = 0b00000110;
constexpr std::uint8_t TYPE_OTHER = 0b00000111;
static_assert(TYPE_SHARED_STRING <= uint_max<type_size>);
static_assert(TYPE_STRING <= uint_max<type_size>);
static_assert(TYPE_LONG_STRING <= uint_max<type_size>);
static_assert(TYPE_OBJECT <= uint_max<type_size>);
static_assert(TYPE_ARRAY <= uint_max<type_size>);
static_assert(TYPE_POSITIVE_INTEGER_BYTE <= uint_max<type_size>);
static_assert(TYPE_NEGATIVE_INTEGER_BYTE <= uint_max<type_size>);
static_assert(TYPE_OTHER <= uint_max<type_size>);

constexpr auto subtype_size = 5;
constexpr std::uint8_t SUBTYPE_FALSE = 0b00000000;
constexpr std::uint8_t SUBTYPE_TRUE = 0b00000001;
constexpr std::uint8_t SUBTYPE_NULL = 0b00000010;
constexpr std::uint8_t SUBTYPE_POSITIVE_INTEGER = 0b00000011;
constexpr std::uint8_t SUBTYPE_NEGATIVE_INTEGER = 0b00000100;
constexpr std::uint8_t SUBTYPE_NUMBER = 0b00000101;
constexpr std::uint8_t SUBTYPE_POSITIVE_REAL_INTEGER_BYTE = 0b00000110;
constexpr std::uint8_t SUBTYPE_LONG_STRING_BASE_EXPONENT_7 = 0b00000111;
constexpr std::uint8_t SUBTYPE_LONG_STRING_BASE_EXPONENT_8 = 0b00001000;
constexpr std::uint8_t SUBTYPE_LONG_STRING_BASE_EXPONENT_9 = 0b00001001;
constexpr std::uint8_t SUBTYPE_LONG_STRING_BASE_EXPONENT_10 = 0b00001010;

static_assert(SUBTYPE_FALSE <= uint_max<subtype_size>);
static_assert(SUBTYPE_TRUE <= uint_max<subtype_size>);
static_assert(SUBTYPE_NULL <= uint_max<subtype_size>);
static_assert(SUBTYPE_POSITIVE_INTEGER <= uint_max<subtype_size>);
static_assert(SUBTYPE_NEGATIVE_INTEGER <= uint_max<subtype_size>);
static_assert(SUBTYPE_NUMBER <= uint_max<subtype_size>);
static_assert(SUBTYPE_POSITIVE_REAL_INTEGER_BYTE <= uint_max<subtype_size>);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_7 <= uint_max<subtype_size>);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_8 <= uint_max<subtype_size>);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_9 <= uint_max<subtype_size>);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_10 <= uint_max<subtype_size>);

// Note that the binary values actually match the declared exponents
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_7 == 7);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_8 == 8);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_9 == 9);
static_assert(SUBTYPE_LONG_STRING_BASE_EXPONENT_10 == 10);
} // namespace internal::ANY_PACKED_TYPE_TAG_BYTE_PREFIX
#endif

/// @}

/// @ingroup runtime
/// @defgroup encoding_string String Encodings
/// @{

// clang-format off
/// @brief The encoding consist in the UTF-8 encoding of the input string.
///
/// ### Options
///
/// | Option | Type   | Description                  |
/// |--------|--------|------------------------------|
/// | `size` | `uint` | The string UTF-8 byte-length |
///
/// ### Conditions
///
/// | Condition            | Description                                               |
/// |----------------------|-----------------------------------------------------------|
/// | `len(value) == size` | The input string must have the declared UTF-8 byte-length |
///
/// ### Examples
///
/// Given the input value "foo bar" with a corresponding size of 7, the encoding
/// results in:
///
/// ```
/// +------+------+------+------+------+------+------+
/// | 0x66 | 0x6f | 0x6f | 0x20 | 0x62 | 0x61 | 0x72 |
/// +------+------+------+------+------+------+------+
///   f      o      o             b      a      r
/// ```
// clang-format on
struct UTF8_STRING_NO_LENGTH {
  /// The string UTF-8 byte-length
  const std::uint64_t size;
};

// clang-format off
/// @brief The encoding consists of the byte-length of the string minus
/// `minimum` plus 1 as a Base-128 64-bit Little Endian variable-length
/// unsigned integer followed by the UTF-8 encoding of the input value.
///
/// Optionally, if the input string has already been encoded to the buffer
/// using UTF-8, the encoding may consist of the byte constant `0x00` followed
/// by the byte-length of the string minus `minimum` plus 1 as a Base-128
/// 64-bit Little Endian variable-length unsigned integer, followed by the
/// current offset minus the offset to the start of the UTF-8 string value in
/// the buffer encoded as a Base-128 64-bit Little Endian variable-length
/// unsigned integer.
///
/// #### Options
///
/// | Option    | Type   | Description                                    |
/// |-----------|--------|------------------------------------------------|
/// | `minimum` | `uint` | The inclusive minimum string UTF-8 byte-length |
///
/// #### Conditions
///
/// | Condition               | Description                                                          |
/// |-------------------------|----------------------------------------------------------------------|
/// | `len(value) >= minimum` | The input string byte-length is equal to or greater than the minimum |
///
/// #### Examples
///
/// Given the input string `foo` with a minimum 3 where the string has not been
/// previously encoded, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x01 | 0x66 | 0x6f | 0x6f |
/// +------+------+------+------+
///          f      o      o
/// ```
///
/// Given the encoding of `foo` with a minimum of 0 followed by the encoding of
/// `foo` with a minimum of 3, the encoding may result in:
///
/// ```
/// 0      1      2      3      4      5      6
/// ^      ^      ^      ^      ^      ^      ^
/// +------+------+------+------+------+------+------+
/// | 0x04 | 0x66 | 0x6f | 0x6f | 0x00 | 0x01 | 0x05 |
/// +------+------+------+------+------+------+------+
///          f      o      o                    6 - 1
/// ```
// clang-format on
struct FLOOR_VARINT_PREFIX_UTF8_STRING_SHARED {
  /// The inclusive minimum string UTF-8 byte-length
  const std::uint64_t minimum;
};

// clang-format off
/// @brief The encoding consists of `maximum` minus the byte-length of the
/// string plus 1 as a Base-128 64-bit Little Endian variable-length unsigned
/// integer followed by the UTF-8 encoding of the input value.
///
/// Optionally, if the input string has already been encoded to the buffer
/// using UTF-8, the encoding may consist of the byte constant `0x00` followed
/// by `maximum` minus the byte-length of the string plus 1 as a Base-128
/// 64-bit Little Endian variable-length unsigned integer, followed by the
/// current offset minus the offset to the start of the UTF-8 string value in
/// the buffer encoded as a Base-128 64-bit Little Endian variable-length
/// unsigned integer.
///
/// #### Options
///
/// | Option    | Type   | Description                                    |
/// |-----------|--------|------------------------------------------------|
/// | `maximum` | `uint` | The inclusive maximum string UTF-8 byte-length |
///
/// #### Conditions
///
/// | Condition               | Description                                                       |
/// |-------------------------|-------------------------------------------------------------------|
/// | `len(value) <= maximum` | The input string byte-length is equal to or less than the maximum |
///
/// #### Examples
///
/// Given the input string `foo` with a maximum 4 where the string has not been
/// previously encoded, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x02 | 0x66 | 0x6f | 0x6f |
/// +------+------+------+------+
///          f      o      o
/// ```
///
/// Given the encoding of `foo` with a maximum of 3 followed by the encoding of
/// `foo` with a maximum of 5, the encoding may result in:
///
/// ```
/// 0      1      2      3      4      5      6
/// ^      ^      ^      ^      ^      ^      ^
/// +------+------+------+------+------+------+------+
/// | 0x01 | 0x66 | 0x6f | 0x6f | 0x00 | 0x03 | 0x05 |
/// +------+------+------+------+------+------+------+
///          f      o      o                    6 - 1
/// ```
// clang-format on
struct ROOF_VARINT_PREFIX_UTF8_STRING_SHARED {
  /// The inclusive maximum string UTF-8 byte-length
  const std::uint64_t maximum;
};

// clang-format off
/// @brief The encoding consists of the byte-length of the string minus
/// `minimum` plus 1 as an 8-bit fixed-length unsigned integer followed by the
/// UTF-8 encoding of the input value.
///
/// Optionally, if the input string has already been encoded to the buffer
/// using UTF-8, the encoding may consist of the byte constant `0x00` followed
/// by the byte-length of the string minus `minimum` plus 1 as an 8-bit
/// fixed-length unsigned integer, followed by the current offset minus the
/// offset to the start of the UTF-8 string value in the buffer encoded as a
/// Base-128 64-bit Little Endian variable-length unsigned integer.
///
/// The byte-length of the string is encoded even if `maximum` equals `minimum`
/// in order to disambiguate between shared and non-shared fixed strings.
///
/// #### Options
///
/// | Option    | Type   | Description                                    |
/// |-----------|--------|------------------------------------------------|
/// | `minimum` | `uint` | The inclusive minimum string UTF-8 byte-length |
/// | `maximum` | `uint` | The inclusive maximum string UTF-8 byte-length |
///
/// #### Conditions
///
/// | Condition                        | Description                                                          |
/// |----------------------------------|----------------------------------------------------------------------|
/// | `len(value) >= minimum`          | The input string byte-length is equal to or greater than the minimum |
/// | `len(value) <= maximum`          | The input string byte-length is equal to or less than the maximum    |
/// | `maximum - minimum < 2 ** 8 - 1` | The range minus 1 must be representable in 8 bits                    |
///
/// #### Examples
///
/// Given the input string `foo` with a minimum 3 and a maximum 5 where the string
/// has not been previously encoded, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x01 | 0x66 | 0x6f | 0x6f |
/// +------+------+------+------+
///          f      o      o
/// ```
///
/// Given the encoding of `foo` with a minimum of 0 and a maximum of 6 followed by
/// the encoding of `foo` with a minimum of 3 and a maximum of 100, the encoding
/// may result in:
///
/// ```
/// 0      1      2      3      4      5      6
/// ^      ^      ^      ^      ^      ^      ^
/// +------+------+------+------+------+------+------+
/// | 0x04 | 0x66 | 0x6f | 0x6f | 0x00 | 0x01 | 0x05 |
/// +------+------+------+------+------+------+------+
///          f      o      o                    6 - 1
/// ```
// clang-format on
struct BOUNDED_8BIT_PREFIX_UTF8_STRING_SHARED {
  /// The inclusive minimum string UTF-8 byte-length
  const std::uint64_t minimum;
  /// The inclusive maximum string UTF-8 byte-length
  const std::uint64_t maximum;
};

// clang-format off
/// @brief The encoding consists of an implementation of
/// [RFC3339](https://datatracker.ietf.org/doc/html/rfc3339) date expressions
/// as the sequence of 3 integers: the year as a 16-bit fixed-length Little
/// Endian unsigned integer, the month as an 8-bit fixed-length unsigned
/// integer, and the day as an 8-bit fixed-length unsigned integer.
///
/// #### Options
///
/// None
///
/// #### Conditions
///
/// | Condition            | Description                                                 |
/// |----------------------|-------------------------------------------------------------|
/// | `len(value) == 10`   | The input string consists of 10 characters                  |
/// | `value[0:4] >= 0`    | The year is greater than or equal to 0                      |
/// | `value[0:4] <= 9999` | The year is less than or equal to 9999 as stated by RFC3339 |
/// | `value[4] == "-"`    | The year and the month are divided by a hyphen              |
/// | `value[5:7] >= 1`    | The month is greater than or equal to 1                     |
/// | `value[5:7] <= 12`   | The month is less than or equal to 12                       |
/// | `value[7] == "-"`    | The month and the day are divided by a hyphen               |
/// | `value[8:10] >= 1`   | The day is greater than or equal to 1                       |
/// | `value[8:10] <= 31`  | The day is less than or equal to 31                         |
///
/// #### Examples
///
/// Given the input string `2014-10-01`, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0xde | 0x07 | 0x0a | 0x01 |
/// +------+------+------+------+
///   year   ...    month  day
/// ```
// clang-format on
struct RFC3339_DATE_INTEGER_TRIPLET {};

// clang-format off
/// @brief The encoding consists of the byte-length of the string plus 1 as a
/// Base-128 64-bit Little Endian variable-length unsigned integer followed by
/// the UTF-8 encoding of the input value.
///
/// Optionally, if the input string has already been encoded to the buffer
/// using this encoding the encoding may consist of the byte constant `0x00`
/// followed by the current offset minus the offset to the start of the string
/// as a Base-128 64-bit Little Endian variable-length unsigned integer.  It is
/// permissible to point to another instance of the string that is a pointer
/// itself.
///
/// ### Options
///
/// None
///
/// ### Conditions
///
/// None
///
/// ### Examples
///
/// Given the input string `foo` where the string has not been previously encoded,
/// the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x04 | 0x66 | 0x6f | 0x6f |
/// +------+------+------+------+
///          f      o      o
/// ```
///
/// Given the encoding of `foo` repeated 3 times, the encoding may result in:
///
/// ```
/// 0      1      2      3      4      5      6      7
/// ^      ^      ^      ^      ^      ^      ^      ^
/// +------+------+------+------+------+------+------+------+
/// | 0x04 | 0x66 | 0x6f | 0x6f | 0x00 | 0x05 | 0x00 | 0x03 |
/// +------+------+------+------+------+------+------+------+
///          f      o      o             5 - 0         7 - 4
/// ```
// clang-format on
struct PREFIX_VARINT_LENGTH_STRING_SHARED {};

/// @}

/// @ingroup runtime
/// @defgroup encoding_array Array Encodings
/// @{

// clang-format off
/// @brief The encoding consists of the elements of the fixed array encoded in
/// order. The encoding of the element at index `i` is either
/// `prefix_encodings[i]` if set, or `encoding`.
///
/// ### Options
///
/// | Option            | Type         | Description          |
/// |-------------------|--------------|----------------------|
/// | `size`            | `uint`       | The array length     |
/// | `prefixEncodings` | `encoding[]` | Positional encodings |
/// | `encoding`        | `encoding`   | Element encoding     |
///
/// ### Conditions
///
/// | Condition                      | Description                                                           |
/// |--------------------------------|-----------------------------------------------------------------------|
/// | `len(prefixEncodings) <= size` | The number of prefix encodings must be less than or equal to the size |
/// | `len(value) == size`           | The input array must have the declared size                           |
///
/// ### Examples
///
/// Given the array `[ 1, 2, true ]` where the `prefixEncodings` corresponds to
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED (minimum 0, maximum 10, multiplier 1) and
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED (minimum 0, maximum 10, multiplier 1) and
/// `encoding` corresponds to BYTE_CHOICE_INDEX with choices `[ false, true ]`,
/// the encoding results in:
///
/// ```
/// +------+------+------+
/// | 0x00 | 0x01 | 0x01 |
/// +------+------+------+
///   1      2      true
/// ```
// clang-format on
struct FIXED_TYPED_ARRAY {
  /// The array length
  const std::uint64_t size;
  /// Element encoding
  const std::shared_ptr<Encoding> encoding;
  /// Positional encodings
  const std::vector<Encoding> prefix_encodings;
};

// clang-format off
/// @brief The encoding consists of the length of the array minus `minimum`
/// encoded as an 8-bit fixed-length unsigned integer followed by the elements
/// of the array encoded in order. The encoding of the element at index `i` is
/// either `prefix_encodings[i]` if set, or `encoding`.
///
/// ### Options
///
/// | Option            | Type         | Description                     |
/// |-------------------|--------------|---------------------------------|
/// | `minimum`         | `uint`       | The minimum length of the array |
/// | `maximum`         | `uint`       | The maximum length of the array |
/// | `prefixEncodings` | `encoding[]` | Positional encodings            |
/// | `encoding`        | `encoding`   | Element encoding                |
///
/// ### Conditions
///
/// | Condition                              | Description                                                                           |
/// |----------------------------------------|---------------------------------------------------------------------------------------|
/// | `len(value) >= minimum`                | The length of the array must be greater than or equal to the minimum                  |
/// | `len(value) <= maximum`                | The length of the array must be less than or equal to the maximum                     |
/// | `len(prefixEncodings) <= maximum`      | The number of prefix encodings must be less than or equal to the maximum array length |
/// | `len(maximum) - len(minimum) < 2 ** 8` | The array length must be representable in 8 bits                                      |
///
/// ### Examples
///
/// Given the array `[ true, false, 5 ]` where the minimum is 1 and the maximum is
/// 3, the `prefixEncodings` corresponds to BYTE_CHOICE_INDEX with
/// choices `[ false, true ]` and BYTE_CHOICE_INDEX with choices `[
/// false, true ]` and `encoding` corresponds to
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED with minimum 0 and maximum
/// 255, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x02 | 0x01 | 0x00 | 0x05 |
/// +------+------+------+------+
///   size   true   false  5
/// ```
// clang-format on
struct BOUNDED_8BITS_TYPED_ARRAY {
  /// The minimum length of the array
  const std::uint64_t minimum;
  /// The maximum length of the array
  const std::uint64_t maximum;
  /// Element encoding
  const std::shared_ptr<Encoding> encoding;
  /// Positional encodings
  const std::vector<Encoding> prefix_encodings;
};

// clang-format off
/// @brief The encoding consists of the length of the array minus `minimum`
/// encoded as a Base-128 64-bit Little Endian variable-length unsigned integer
/// followed by the elements of the array encoded in order. The encoding of the
/// element at index `i` is either `prefix_encodings[i]` if set, or `encoding`.
///
/// ### Options
///
/// | Option            | Type         | Description                     |
/// |-------------------|--------------|---------------------------------|
/// | `minimum`         | `uint`       | The minimum length of the array |
/// | `prefixEncodings` | `encoding[]` | Positional encodings            |
/// | `encoding`        | `encoding`   | Element encoding                |
///
/// ### Conditions
///
/// | Condition               | Description                                                          |
/// |-------------------------|----------------------------------------------------------------------|
/// | `len(value) >= minimum` | The length of the array must be greater than or equal to the minimum |
///
/// ### Examples
///
/// TODO: Give an example of an array with more than 8-bit of elements
///
/// Given the array `[ true, false, 5 ]` where the minimum is 1, the
/// `prefixEncodings` corresponds to BYTE_CHOICE_INDEX with choices `[
/// false, true ]` and BYTE_CHOICE_INDEX with choices `[ false, true ]`
/// and `encoding` corresponds to BOUNDED_MULTIPLE_8BITS_ENUM_FIXED
/// with minimum 0 and maximum 255, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x02 | 0x01 | 0x00 | 0x05 |
/// +------+------+------+------+
///   size   true   false  5
/// ```
// clang-format on
struct FLOOR_TYPED_ARRAY {
  /// The minimum length of the array
  const std::uint64_t minimum;
  /// Element encoding
  const std::shared_ptr<Encoding> encoding;
  /// Positional encodings
  const std::vector<Encoding> prefix_encodings;
};

// clang-format off
/// @brief The encoding consists of `maximum` minus the length of the array
/// encoded as a Base-128 64-bit Little Endian variable-length unsigned integer
/// followed by the elements of the array encoded in order. The encoding of the
/// element at index `i` is either `prefix_encodings[i]` if set, or `encoding`.
///
/// ### Options
///
/// | Option            | Type         | Description                     |
/// |-------------------|--------------|---------------------------------|
/// | `maximum`         | `uint`       | The maximum length of the array |
/// | `prefixEncodings` | `encoding[]` | Positional encodings            |
/// | `encoding`        | `encoding`   | Element encoding                |
///
/// ### Conditions
///
/// | Condition                         | Description                                                                           |
/// |-----------------------------------|---------------------------------------------------------------------------------------|
/// | `len(prefixEncodings) <= maximum` | The number of prefix encodings must be less than or equal to the maximum array length |
///
/// ### Examples
///
/// Given the array `[ true, false, 5 ]` where the maximum is 3, the
/// `prefixEncodings` options corresponds to BYTE_CHOICE_INDEX with
/// choices `[ false, true ]` and BYTE_CHOICE_INDEX with choices `[
/// false, true ]` and `encoding` corresponds to
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED with minimum 0 and maximum
/// 255, the encoding results in:
///
/// ```
/// +------+------+------+------+
/// | 0x00 | 0x01 | 0x00 | 0x05 |
/// +------+------+------+------+
///   size   true   false  5
/// ```
// clang-format on
struct ROOF_TYPED_ARRAY {
  /// The maximum length of the array
  const std::uint64_t maximum;
  /// Element encoding
  const std::shared_ptr<Encoding> encoding;
  /// Positional encodings
  const std::vector<Encoding> prefix_encodings;
};

/// @}

/// @ingroup runtime
/// @defgroup encoding_object Object Encodings
/// @{

// clang-format off
/// @brief The encoding consists of each pair encoded as the key followed by
/// the value according to `key_encoding` and `encoding`. The order in which
/// pairs are encoded is undefined.
///
/// ### Options
///
/// | Option        | Type       | Description     |
/// |---------------|------------|-----------------|
/// | `size`        | `uint`     | The object size |
/// | `keyEncoding` | `encoding` | Key encoding    |
/// | `encoding`    | `encoding` | Value encoding  |
///
/// ### Conditions
///
/// | Condition            | Description                                               |
/// |----------------------|-----------------------------------------------------------|
/// | `len(value) == size` | The input object must have the declared amount of entries |
///
/// ### Examples
///
/// Given the array `{ "foo": 1, "bar": 2 }` where `keyEncoding` corresponds to
/// UTF8_STRING_NO_LENGTH (size 3) and `encoding` corresponds to
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED (minimum 0, maximum 10,
/// multiplier 1), the encoding results in:
///
/// ```
/// +------+------+------+------+------+------+------+------+
/// | 0x66 | 0x6f | 0x6f | 0x01 | 0x62 | 0x61 | 0x72 | 0x02 |
/// +------+------+------+------+------+------+------+------+
///   f      o      o      1      b      a      r      2
/// ```
///
/// Or:
///
/// ```
/// +------+------+------+------+------+------+------+------+
/// | 0x62 | 0x61 | 0x72 | 0x02 | 0x66 | 0x6f | 0x6f | 0x01 |
/// +------+------+------+------+------+------+------+------+
///   b      a      r      2      f      o      o      1
/// ```
// clang-format on
struct FIXED_TYPED_ARBITRARY_OBJECT {
  /// The object size
  const std::uint64_t size;
  /// Key encoding
  const std::shared_ptr<Encoding> key_encoding;
  /// Value encoding
  const std::shared_ptr<Encoding> encoding;
};

// clang-format off
/// @brief The encoding consists of the number of key-value pairs in the input
/// object as a Base-128 64-bit Little Endian variable-length unsigned integer
/// followed by each pair encoded as the key followed by the value according to
/// `key_encoding` and `encoding`. The order in which pairs are encoded is
/// undefined.
///
/// ### Options
///
/// | Option        | Type       | Description    |
/// |---------------|------------|----------------|
/// | `keyEncoding` | `encoding` | Key encoding   |
/// | `encoding`    | `encoding` | Value encoding |
///
/// ### Examples
///
/// Given the array `{ "foo": 1, "bar": 2 }` where `keyEncoding` corresponds to
/// UTF8_STRING_NO_LENGTH (size 3) and `encoding` corresponds to
/// BOUNDED_MULTIPLE_8BITS_ENUM_FIXED (minimum 0, maximum 10,
/// multiplier 1), the encoding results in:
///
/// ```
/// +------+------+------+------+------+------+------+------+------+
/// | 0x02 | 0x66 | 0x6f | 0x6f | 0x01 | 0x62 | 0x61 | 0x72 | 0x02 |
/// +------+------+------+------+------+------+------+------+------+
///   2      f      o      o      1      b      a      r      2
/// ```
///
/// Or:
///
/// ```
/// +------+------+------+------+------+------+------+------+------+
/// | 0x02 | 0x62 | 0x61 | 0x72 | 0x02 | 0x66 | 0x6f | 0x6f | 0x01 |
/// +------+------+------+------+------+------+------+------+------+
///   2      b      a      r      2      f      o      o      1
/// ```
// clang-format on
struct VARINT_TYPED_ARBITRARY_OBJECT {
  /// Key encoding
  const std::shared_ptr<Encoding> key_encoding;
  /// Value encoding
  const std::shared_ptr<Encoding> encoding;
};

/// @}

} // namespace sourcemeta::jsonbinpack

#endif
