#include <sourcemeta/core/numeric_decimal.h>
#include <sourcemeta/core/numeric_error.h>

#include <cassert>     // assert
#include <cmath>       // std::isfinite
#include <cstdint>     // std::int64_t, std::uint64_t
#include <cstring>     // std::strlen
#include <iomanip>     // std::setprecision
#include <limits>      // std::numeric_limits
#include <new>         // new
#include <sstream>     // std::ostringstream
#include <stdexcept>   // std::out_of_range
#include <string>      // std::string, std::stof, std::stod
#include <type_traits> // std::is_same_v
#include <utility>     // std::move

#include <mpdecimal.h> // mpd_*

namespace {
template <typename FloatingPointType>
  requires std::is_floating_point_v<FloatingPointType>
auto floating_point_to_string(const FloatingPointType value) -> std::string {
  std::ostringstream oss;
  oss << std::setprecision(std::numeric_limits<FloatingPointType>::max_digits10)
      << value;
  return oss.str();
}
} // namespace

namespace sourcemeta::core {

struct Decimal::Data {
  static constexpr mpd_ssize_t STATIC_BUFFER_SIZE{4};
  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
  mpd_uint_t buffer[STATIC_BUFFER_SIZE];
  mpd_t value;
};

auto Decimal::data() -> Data * {
  return reinterpret_cast<Data *>(this->storage);
}

auto Decimal::data() const -> const Data * {
  static_assert(sizeof(Data) <= STORAGE_SIZE,
                "Storage size too small for Decimal::Data");
  static_assert(alignof(Data) <= alignof(std::max_align_t),
                "Data alignment exceeds max_align_t");
  return reinterpret_cast<const Data *>(this->storage);
}

} // namespace sourcemeta::core

namespace {

// One-time global initialization of MPD_MINALLOC
// This must happen before any thread-local contexts are initialized
[[maybe_unused]] static const bool minalloc_initialized = []() {
  constexpr mpd_ssize_t precision{16};
  const mpd_ssize_t ideal_minalloc =
      2 * ((precision + MPD_RDIGITS - 1) / MPD_RDIGITS);
  mpd_setminalloc(ideal_minalloc);
  return true;
}();

// Thread-local context for decimal arithmetic operations
// Matches the C++ wrapper context_template settings (16 digit precision)
// Note: We use mpd_defaultcontext + mpd_qsetprec instead of mpd_init
// to avoid calling mpd_setminalloc multiple times (once per thread)
thread_local mpd_context_t decimal_context = []() {
  mpd_context_t context;
  mpd_defaultcontext(&context);
  mpd_qsetprec(&context, 16);
  context.emax = 999999;
  context.emin = -999999;
  context.round = MPD_ROUND_HALF_EVEN;
  context.traps =
      MPD_IEEE_Invalid_operation | MPD_Division_by_zero | MPD_Overflow;
  return context;
}();

// Thread-local max precision context for string I/O operations
thread_local mpd_context_t max_context = []() {
  mpd_context_t context;
  mpd_maxcontext(&context);
  return context;
}();

template <typename FloatingPointType>
auto is_representable_as_floating_point(
    const mpd_t *const mpd_value, const sourcemeta::core::Decimal &decimal)
    -> bool {
  if (mpd_isnan(mpd_value) || mpd_isinfinite(mpd_value)) {
    return true;
  }

  if (!mpd_isfinite(mpd_value)) {
    return false;
  }

  const std::string decimal_string{decimal.to_scientific_string()};
  FloatingPointType converted_value;
  try {
    if constexpr (std::is_same_v<FloatingPointType, float>) {
      converted_value = std::stof(decimal_string);
    } else if constexpr (std::is_same_v<FloatingPointType, double>) {
      converted_value = std::stod(decimal_string);
    }
  } catch (const std::out_of_range &) {
    return false;
  }

  if (!std::isfinite(converted_value)) {
    return false;
  }

  std::ostringstream oss;
  oss << std::setprecision(std::numeric_limits<FloatingPointType>::max_digits10)
      << converted_value;
  const sourcemeta::core::Decimal roundtrip{oss.str()};
  return decimal == roundtrip;
}

} // namespace

namespace sourcemeta::core {

Decimal::Decimal() noexcept {
  new (this->storage) Data{};
  this->data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
  this->data()->value.exp = 0;
  this->data()->value.digits = 1;
  this->data()->value.len = 1;
  this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
  this->data()->value.data = this->data()->buffer;
  this->data()->buffer[0] = 0;
}

Decimal::~Decimal() {
  if (this->data()->value.data != this->data()->buffer) {
    mpd_free(this->data()->value.data);
  }

  this->data()->~Data();
}

Decimal::Decimal(const Decimal &other) {
  new (this->storage) Data{};
  this->data()->value.flags =
      (other.data()->value.flags & MPD_NEG) | MPD_STATIC | MPD_STATIC_DATA;
  this->data()->value.exp = 0;
  this->data()->value.digits = 0;
  this->data()->value.len = 0;
  this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
  this->data()->value.data = this->data()->buffer;

  std::uint32_t status = 0;
  if (!mpd_qcopy(&this->data()->value, &other.data()->value, &status)) {
    throw NumericOutOfMemoryError{};
  }
}

Decimal::Decimal(Decimal &&other) noexcept {
  new (this->storage) Data{};
  if (other.data()->value.data == other.data()->buffer) {
    // Other uses static storage, copy the data
    this->data()->value.flags =
        (other.data()->value.flags & MPD_NEG) | MPD_STATIC | MPD_STATIC_DATA;
    this->data()->value.exp = other.data()->value.exp;
    this->data()->value.digits = other.data()->value.digits;
    this->data()->value.len = other.data()->value.len;
    this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
    this->data()->value.data = this->data()->buffer;
    this->data()->buffer[0] = other.data()->buffer[0];
    this->data()->buffer[1] = other.data()->buffer[1];
    this->data()->buffer[2] = other.data()->buffer[2];
    this->data()->buffer[3] = other.data()->buffer[3];
  } else {
    // Other uses dynamic storage, steal it
    this->data()->value = other.data()->value;
    this->data()->value.flags |= MPD_STATIC;
    // Reset other to zero with static storage
    other.data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
    other.data()->value.exp = 0;
    other.data()->value.digits = 1;
    other.data()->value.len = 1;
    other.data()->value.alloc = Data::STATIC_BUFFER_SIZE;
    other.data()->value.data = other.data()->buffer;
    other.data()->buffer[0] = 0;
  }
}

auto Decimal::operator=(const Decimal &other) -> Decimal & {
  if (this != &other) {
    std::uint32_t status = 0;
    if (!mpd_qcopy(&this->data()->value, &other.data()->value, &status)) {
      throw NumericOutOfMemoryError{};
    }
  }

  return *this;
}

auto Decimal::operator=(Decimal &&other) noexcept -> Decimal & {
  if (this != &other) {
    // Free our dynamic data if we have any
    if (this->data()->value.data != this->data()->buffer) {
      mpd_free(this->data()->value.data);
    }

    if (other.data()->value.data == other.data()->buffer) {
      // Other uses static storage, copy the data
      this->data()->value.flags =
          (other.data()->value.flags & MPD_NEG) | MPD_STATIC | MPD_STATIC_DATA;
      this->data()->value.exp = other.data()->value.exp;
      this->data()->value.digits = other.data()->value.digits;
      this->data()->value.len = other.data()->value.len;
      this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
      this->data()->value.data = this->data()->buffer;
      this->data()->buffer[0] = other.data()->buffer[0];
      this->data()->buffer[1] = other.data()->buffer[1];
      this->data()->buffer[2] = other.data()->buffer[2];
      this->data()->buffer[3] = other.data()->buffer[3];
    } else {
      // Other uses dynamic storage, steal it
      this->data()->value = other.data()->value;
      this->data()->value.flags |= MPD_STATIC;
      // Reset other to zero with static storage
      other.data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
      other.data()->value.exp = 0;
      other.data()->value.digits = 1;
      other.data()->value.len = 1;
      other.data()->value.alloc = Data::STATIC_BUFFER_SIZE;
      other.data()->value.data = other.data()->buffer;
      other.data()->buffer[0] = 0;
    }
  }

  return *this;
}

Decimal::Decimal(const std::int64_t integral_value) {
  new (this->storage) Data{};
  this->data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
  this->data()->value.exp = 0;
  this->data()->value.digits = 0;
  this->data()->value.len = 0;
  this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
  this->data()->value.data = this->data()->buffer;

  std::uint32_t status = 0;
  mpd_qset_i64(&this->data()->value, integral_value, &decimal_context, &status);

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }
}

Decimal::Decimal(const std::uint64_t integral_value) {
  new (this->storage) Data{};
  this->data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
  this->data()->value.exp = 0;
  this->data()->value.digits = 0;
  this->data()->value.len = 0;
  this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
  this->data()->value.data = this->data()->buffer;

  std::uint32_t status = 0;
  mpd_qset_u64(&this->data()->value, integral_value, &decimal_context, &status);

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }
}

Decimal::Decimal(const float floating_point_value)
    : Decimal{floating_point_to_string(floating_point_value)} {}

Decimal::Decimal(const double floating_point_value)
    : Decimal{floating_point_to_string(floating_point_value)} {}

Decimal::Decimal(const char *const string_value) {
  new (this->storage) Data{};
  this->data()->value.flags = MPD_STATIC | MPD_STATIC_DATA;
  this->data()->value.exp = 0;
  this->data()->value.digits = 0;
  this->data()->value.len = 0;
  this->data()->value.alloc = Data::STATIC_BUFFER_SIZE;
  this->data()->value.data = this->data()->buffer;

  std::uint32_t status = 0;
  mpd_qset_string(&this->data()->value, string_value, &max_context, &status);

  if (status & MPD_Conversion_syntax) {
    throw DecimalParseError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }
}

Decimal::Decimal(const std::string &string_value)
    : Decimal{string_value.c_str()} {}

Decimal::Decimal(const std::string_view string_value)
    : Decimal{std::string{string_value}} {}

auto Decimal::nan() -> Decimal {
  Decimal result;
  mpd_setspecial(&result.data()->value, MPD_POS, MPD_NAN);
  return result;
}

auto Decimal::infinity() -> Decimal {
  Decimal result;
  mpd_setspecial(&result.data()->value, MPD_POS, MPD_INF);
  return result;
}

auto Decimal::negative_infinity() -> Decimal {
  Decimal result;
  mpd_setspecial(&result.data()->value, MPD_NEG, MPD_INF);
  return result;
}

auto Decimal::to_scientific_string() const -> std::string {
  // Note that `mpd_to_sci`, contrary to its name, does NOT guarantee
  // we get exponential notation. It still avoids it for small-ish numbers
  char *result_string = mpd_format(&this->data()->value, "e", &max_context);
  if (result_string == nullptr) {
    throw NumericOutOfMemoryError{};
  }

  std::string result{result_string};
  mpd_free(result_string);
  return result;
}

auto Decimal::to_string() const -> std::string {
  // Use mpd_to_eng for plain notation without scientific format
  char *result_string = mpd_to_eng(&this->data()->value, 0);
  if (result_string == nullptr) {
    throw NumericOutOfMemoryError{};
  }

  std::string result{result_string};
  mpd_free(result_string);
  return result;
}

auto Decimal::to_int64() const -> std::int64_t {
  assert(this->is_int64());
  std::int64_t result =
      mpd_qget_i64(&this->data()->value, &decimal_context.status);
  return result;
}

auto Decimal::to_int32() const -> std::int32_t {
  assert(this->is_int32());
  std::int32_t result =
      mpd_qget_i32(&this->data()->value, &decimal_context.status);
  return result;
}

auto Decimal::to_uint64() const -> std::uint64_t {
  assert(this->is_uint64());
  std::uint64_t result =
      mpd_qget_u64(&this->data()->value, &decimal_context.status);
  return result;
}

auto Decimal::to_uint32() const -> std::uint32_t {
  assert(this->is_uint32());
  std::uint32_t result =
      mpd_qget_u32(&this->data()->value, &decimal_context.status);
  return result;
}

auto Decimal::to_float() const -> float {
  const std::string scientific_notation{this->to_scientific_string()};
  return std::stof(scientific_notation);
}

auto Decimal::to_double() const -> double {
  const std::string scientific_notation{this->to_scientific_string()};
  return std::stod(scientific_notation);
}

auto Decimal::is_zero() const -> bool {
  return mpd_iszero(&this->data()->value);
}

auto Decimal::is_integral() const -> bool {
  return mpd_isinteger(&this->data()->value);
}

auto Decimal::is_integer() const -> bool {
  return this->is_integral() && this->data()->value.exp >= 0;
}

auto Decimal::is_finite() const -> bool {
  return mpd_isfinite(&this->data()->value);
}

auto Decimal::is_real() const -> bool {
  return mpd_isfinite(&this->data()->value) &&
         !mpd_isinteger(&this->data()->value);
}

auto Decimal::is_float() const -> bool {
  return is_representable_as_floating_point<float>(&this->data()->value, *this);
}

auto Decimal::is_double() const -> bool {
  return is_representable_as_floating_point<double>(&this->data()->value,
                                                    *this);
}

auto Decimal::is_int32() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{std::numeric_limits<std::int32_t>::min()} &&
         *this <= Decimal{std::numeric_limits<std::int32_t>::max()};
}

auto Decimal::is_int64() const -> bool {
  assert(this->is_integer());
  // Try to get as int64 with max precision context to check if it fits
  std::uint32_t status = 0;
  mpd_qget_i64(&this->data()->value, &status);
  return !(status & MPD_Invalid_operation);
}

auto Decimal::is_uint32() const -> bool {
  assert(this->is_integer());
  return *this >= Decimal{0} &&
         *this <= Decimal{std::numeric_limits<std::uint32_t>::max()};
}

auto Decimal::is_uint64() const -> bool {
  assert(this->is_integer());
  // Try to get as uint64 to check if it fits
  std::uint32_t status = 0;
  mpd_qget_u64(&this->data()->value, &status);
  return !(status & MPD_Invalid_operation);
}

auto Decimal::is_nan() const -> bool { return mpd_isnan(&this->data()->value); }

auto Decimal::is_infinite() const -> bool {
  return mpd_isinfinite(&this->data()->value);
}

auto Decimal::is_signed() const -> bool {
  return mpd_issigned(&this->data()->value);
}

auto Decimal::to_integral() const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qround_to_int(&result.data()->value, &this->data()->value,
                    &decimal_context, &status);
  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::divisible_by(const Decimal &divisor) const -> bool {
  if (divisor.is_zero()) {
    return false;
  }

  Decimal remainder;
  std::uint32_t status = 0;
  mpd_qrem(&remainder.data()->value, &this->data()->value,
           &divisor.data()->value, &max_context, &status);

  if (status & MPD_Invalid_operation) {
    return false;
  }

  return mpd_iszero(&remainder.data()->value);
}

auto Decimal::operator+=(const Decimal &other) -> Decimal & {
  std::uint32_t status = 0;
  mpd_qadd(&this->data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator-=(const Decimal &other) -> Decimal & {
  std::uint32_t status = 0;
  mpd_qsub(&this->data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator*=(const Decimal &other) -> Decimal & {
  std::uint32_t status = 0;
  mpd_qmul(&this->data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator/=(const Decimal &other) -> Decimal & {
  if (other.is_zero()) {
    if (this->is_zero()) {
      throw NumericInvalidOperationError{};
    }

    throw NumericDivisionByZeroError{};
  }

  std::uint32_t status = 0;
  mpd_qdiv(&this->data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator%=(const Decimal &other) -> Decimal & {
  if (other.is_zero()) {
    throw NumericInvalidOperationError{};
  }

  std::uint32_t status = 0;
  mpd_qrem(&this->data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Invalid_operation) {
    throw NumericInvalidOperationError{};
  }

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator+(const Decimal &other) const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qadd(&result.data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator-(const Decimal &other) const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qsub(&result.data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator*(const Decimal &other) const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qmul(&result.data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator/(const Decimal &other) const -> Decimal {
  if (other.is_zero()) {
    if (this->is_zero()) {
      throw NumericInvalidOperationError{};
    }

    throw NumericDivisionByZeroError{};
  }

  Decimal result;
  std::uint32_t status = 0;
  mpd_qdiv(&result.data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator%(const Decimal &other) const -> Decimal {
  if (other.is_zero()) {
    throw NumericInvalidOperationError{};
  }

  Decimal result;
  std::uint32_t status = 0;
  mpd_qrem(&result.data()->value, &this->data()->value, &other.data()->value,
           &decimal_context, &status);

  if (status & MPD_Invalid_operation) {
    throw NumericInvalidOperationError{};
  }

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator-() const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qminus(&result.data()->value, &this->data()->value, &decimal_context,
             &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator+() const -> Decimal {
  Decimal result;
  std::uint32_t status = 0;
  mpd_qplus(&result.data()->value, &this->data()->value, &decimal_context,
            &status);

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return result;
}

auto Decimal::operator++() -> Decimal & {
  const Decimal one{1};
  std::uint32_t status = 0;
  mpd_qadd(&this->data()->value, &this->data()->value, &one.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator++(int) -> Decimal {
  Decimal result{*this};
  ++(*this);
  return result;
}

auto Decimal::operator--() -> Decimal & {
  const Decimal one{1};
  std::uint32_t status = 0;
  mpd_qsub(&this->data()->value, &this->data()->value, &one.data()->value,
           &decimal_context, &status);

  if (status & MPD_Overflow) {
    throw NumericOverflowError{};
  }

  if (status & MPD_Malloc_error) {
    throw NumericOutOfMemoryError{};
  }

  return *this;
}

auto Decimal::operator--(int) -> Decimal {
  Decimal result{*this};
  --(*this);
  return result;
}

auto Decimal::operator==(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result == 0;
}

auto Decimal::operator!=(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result != 0;
}

auto Decimal::operator<(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result < 0;
}

auto Decimal::operator<=(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result <= 0;
}

auto Decimal::operator>(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result > 0;
}

auto Decimal::operator>=(const Decimal &other) const -> bool {
  std::uint32_t status = 0;
  const int result =
      mpd_qcmp(&this->data()->value, &other.data()->value, &status);
  return result >= 0;
}

} // namespace sourcemeta::core
