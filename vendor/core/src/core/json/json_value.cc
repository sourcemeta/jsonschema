#include <sourcemeta/core/json_array.h>
#include <sourcemeta/core/json_value.h>

#include <algorithm>        // std::ranges::contains, std::ranges::fold_left
#include <cassert>          // assert
#include <cmath>            // std::isinf, std::isnan, std::modf, std::floor
#include <compare>          // std::strong_ordering, std::is_eq, std::is_lt
#include <cstddef>          // std::size_t
#include <cstdint>          // std::int64_t
#include <exception>        // std::terminate
#include <functional>       // std::reference_wrapper
#include <initializer_list> // std::initializer_list
#include <limits>           // std::numeric_limits
#include <memory>           // std::construct_at
#include <sstream>          // std::basic_istringstream
#include <stdexcept>        // std::invalid_argument
#include <string>           // std::to_string
#include <string_view>      // std::basic_string_view
#include <utility>          // std::exchange, std::move
#include <vector>           // std::vector

// Searching a run of hashes for one value tests many lanes at once on any
// vector unit. The wider of the two x86 extensions is only present when the
// build asks for it, while the narrower one and the ARM vector unit belong to
// every 64-bit baseline, so each is gated on what its architecture guarantees
// rather than on the flags this project happens to pass, which do not reach a
// consumer that builds this library as a subproject of its own
#if defined(__AVX2__)
#define SOURCEMETA_CORE_JSON_SCAN_AVX2 1
#elif defined(__SSE4_1__)
#define SOURCEMETA_CORE_JSON_SCAN_SSE41 1
#elif defined(__aarch64__) || defined(_M_ARM64)
// The horizontal reduction that answers whether any lane matched at all is only
// defined for the 64-bit form of the instruction set
#define SOURCEMETA_CORE_JSON_SCAN_NEON 1
#endif

#if defined(SOURCEMETA_CORE_JSON_SCAN_AVX2) ||                                 \
    defined(SOURCEMETA_CORE_JSON_SCAN_SSE41)
#include <immintrin.h> // __m256i, _mm256_cmpeq_epi64, __m128i, _mm_cmpeq_epi64
#endif

#ifdef SOURCEMETA_CORE_JSON_SCAN_NEON
#include <arm_neon.h> // uint64x2_t, vceqq_u64, vorrq_u64, vmaxvq_u32
#endif

namespace sourcemeta::core {

static constexpr auto TRIM_WHITESPACE = " \t\n\r\v\f";

namespace {

// Report how far into a run of hashes the first one equal to the given value
// lies, or the length of the run when none of them is. The caller has already
// ruled out the first position, so the answer is never zero. The vector loop
// only narrows the search down to the block that holds a match, and the scalar
// tail that follows both pinpoints it within that block and covers the last
// partial one, so a match anywhere in a block costs one test of the whole block
auto scan_hashes(const std::uint64_t *const data, const std::size_t size,
                 const std::uint64_t needle) noexcept -> std::size_t {
  std::size_t offset{0};

#if defined(SOURCEMETA_CORE_JSON_SCAN_AVX2)
  const auto target{_mm256_set1_epi64x(static_cast<long long>(needle))};
  while (offset + 8 <= size) {
    const auto lower{_mm256_cmpeq_epi64(
        _mm256_loadu_si256(reinterpret_cast<const __m256i *>(data + offset)),
        target)};
    const auto upper{
        _mm256_cmpeq_epi64(_mm256_loadu_si256(reinterpret_cast<const __m256i *>(
                               data + offset + 4)),
                           target)};
    const auto matches{_mm256_or_si256(lower, upper)};
    if (_mm256_testz_si256(matches, matches) == 0) {
      break;
    }

    offset += 8;
  }
#elif defined(SOURCEMETA_CORE_JSON_SCAN_SSE41)
  const auto target{_mm_set1_epi64x(static_cast<long long>(needle))};
  while (offset + 8 <= size) {
    const auto first{_mm_cmpeq_epi64(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + offset)),
        target)};
    const auto second{_mm_cmpeq_epi64(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + offset + 2)),
        target)};
    const auto third{_mm_cmpeq_epi64(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + offset + 4)),
        target)};
    const auto fourth{_mm_cmpeq_epi64(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + offset + 6)),
        target)};
    const auto matches{
        _mm_or_si128(_mm_or_si128(first, second), _mm_or_si128(third, fourth))};
    if (_mm_testz_si128(matches, matches) == 0) {
      break;
    }

    offset += 8;
  }
#elif defined(SOURCEMETA_CORE_JSON_SCAN_NEON)
  const auto target{vdupq_n_u64(needle)};
  while (offset + 8 <= size) {
    const auto first{vceqq_u64(vld1q_u64(data + offset), target)};
    const auto second{vceqq_u64(vld1q_u64(data + offset + 2), target)};
    const auto third{vceqq_u64(vld1q_u64(data + offset + 4), target)};
    const auto fourth{vceqq_u64(vld1q_u64(data + offset + 6), target)};
    const auto matches{
        vorrq_u64(vorrq_u64(first, second), vorrq_u64(third, fourth))};
    if (vmaxvq_u32(vreinterpretq_u32_u64(matches)) != 0) {
      break;
    }

    offset += 8;
  }
#endif

  while (offset < size && data[offset] != needle) {
    offset += 1;
  }

  return offset;
}

// Reverse the direction of a comparison result
auto reverse_ordering(const std::strong_ordering ordering)
    -> std::strong_ordering {
  if (std::is_lt(ordering)) {
    return std::strong_ordering::greater;
  }

  if (std::is_gt(ordering)) {
    return std::strong_ordering::less;
  }

  return std::strong_ordering::equal;
}

// Check whether an integer converts to a real without rounding, which IEEE 754
// guarantees only within the 53-bit significand of a binary64
auto integer_fits_real(const std::int64_t value) -> bool {
  constexpr std::int64_t LIMIT{9007199254740992};
  return value >= -LIMIT && value <= LIMIT;
}

// Order a 64-bit integer against a real by exact value, without ever converting
// the integer to a double, which would lose precision beyond 2^53
auto integer_real_ordering(const std::int64_t left, const double right)
    -> std::strong_ordering {
  // Real values in a JSON document are always finite
  constexpr double LOWEST{-9223372036854775808.0};
  constexpr double PAST_HIGHEST{9223372036854775808.0};
  const double floor_of_right{std::floor(right)};
  if (floor_of_right >= PAST_HIGHEST) {
    return std::strong_ordering::less;
  }

  if (floor_of_right < LOWEST) {
    return std::strong_ordering::greater;
  }

  const auto integer_floor{static_cast<std::int64_t>(floor_of_right)};
  if (left != integer_floor) {
    return left < integer_floor ? std::strong_ordering::less
                                : std::strong_ordering::greater;
  }

  // The integer equals the floor of the real, so they are equal only when the
  // real has no fractional part, and otherwise the real is the larger of the
  // two
  return floor_of_right == right ? std::strong_ordering::equal
                                 : std::strong_ordering::less;
}

// Order two numbers held in different representations by exact mathematical
// value. Integer and integral operands are compared without allocating, and
// only a real against a non-integral or out-of-range decimal falls back to the
// exact arbitrary precision expansion
auto cross_numeric_ordering(const JSON &left, const JSON &right)
    -> std::strong_ordering {
  if (left.is_integer() && right.is_real()) {
    return integer_real_ordering(left.to_integer(), right.to_real());
  }

  if (left.is_real() && right.is_integer()) {
    return reverse_ordering(
        integer_real_ordering(right.to_integer(), left.to_real()));
  }

  if (left.is_real() && right.is_decimal() &&
      right.to_decimal().is_integral() && right.to_decimal().is_int64()) {
    return reverse_ordering(
        integer_real_ordering(right.to_decimal().to_int64(), left.to_real()));
  }

  if (left.is_decimal() && right.is_real() && left.to_decimal().is_integral() &&
      left.to_decimal().is_int64()) {
    return integer_real_ordering(left.to_decimal().to_int64(), right.to_real());
  }

  const Decimal left_decimal = left.is_decimal() ? left.to_decimal()
                               : left.is_integer()
                                   ? Decimal{left.to_integer()}
                                   : Decimal::exact_from(left.to_real());
  const Decimal right_decimal = right.is_decimal() ? right.to_decimal()
                                : right.is_integer()
                                    ? Decimal{right.to_integer()}
                                    : Decimal::exact_from(right.to_real());
  if (left_decimal == right_decimal) {
    return std::strong_ordering::equal;
  }

  return left_decimal < right_decimal ? std::strong_ordering::less
                                      : std::strong_ordering::greater;
}

} // namespace

JSON::JSON(const std::int64_t value) : current_type_{Type::Integer} {
  this->data_integer = value;
}

JSON::JSON(const std::size_t value) : current_type_{Type::Integer} {
  this->data_integer = static_cast<Integer>(value);
}

JSON::JSON(const int value) : current_type_{Type::Integer} {
  this->data_integer = value;
}

JSON::JSON(const double value) : current_type_{Type::Real} {
  // Numeric values that cannot be represented as sequences of digits (such as
  // Infinity and NaN) are not permitted. See
  // https://www.ecma-international.org/wp-content/uploads/ECMA-404_2nd_edition_december_2017.pdf
  if (std::isinf(value) || std::isnan(value)) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  this->data_real = value;
}

JSON::JSON(const float value) : JSON(static_cast<Real>(value)) {}

JSON::JSON(const bool value) : current_type_{Type::Boolean} {
  this->data_boolean = value;
}

JSON::JSON(const std::nullptr_t) {}

JSON::JSON(const String &value) : current_type_{Type::String} {
  std::construct_at(&this->data_string, value);
}

JSON::JSON(String &&value) : current_type_{Type::String} {
  std::construct_at(&this->data_string, std::move(value));
}

JSON::JSON(const std::basic_string_view<Char, CharTraits> &value)
    : current_type_{Type::String} {
  std::construct_at(&this->data_string, value);
}

JSON::JSON(const Char *const value) : current_type_{Type::String} {
  std::construct_at(&this->data_string, value);
}

JSON::JSON(const Array &value) : current_type_{Type::Array} {
  std::construct_at(&this->data_array, value);
}

JSON::JSON(std::initializer_list<Object::pair_value_type> values)
    : current_type_{Type::Object} {
  std::construct_at(&this->data_object, values);
}

JSON::JSON(const Object &value) : current_type_{Type::Object} {
  std::construct_at(&this->data_object, value);
}

JSON::JSON(const Decimal &value) : current_type_{Type::Decimal} {
  if (value.is_nan() || value.is_infinite()) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  std::construct_at(&this->data_decimal, value);
}

JSON::JSON(Decimal &&value) : current_type_{Type::Decimal} {
  if (value.is_nan() || value.is_infinite()) {
    throw std::invalid_argument("JSON does not support Infinity or NaN");
  }

  std::construct_at(&this->data_decimal, std::move(value));
}

JSON::JSON(const JSON &other) {
  // Fast path for non-container sources avoids the work-list allocation that
  // would otherwise dominate the cost of copying a scalar value
  switch (other.current_type_) {
    case Type::Null:
      return;
    case Type::Boolean:
      this->data_boolean = other.data_boolean;
      this->current_type_ = Type::Boolean;
      return;
    case Type::Integer:
      this->data_integer = other.data_integer;
      this->current_type_ = Type::Integer;
      return;
    case Type::Real:
      this->data_real = other.data_real;
      this->current_type_ = Type::Real;
      return;
    case Type::String:
      std::construct_at(&this->data_string, other.data_string);
      this->current_type_ = Type::String;
      return;
    case Type::Decimal:
      std::construct_at(&this->data_decimal, other.data_decimal);
      this->current_type_ = Type::Decimal;
      return;
    case Type::Array:
    case Type::Object:
      break;
  }

  // Build the container copy iteratively to avoid unbounded recursion on
  // deeply nested values, which would otherwise overflow the call stack. Each
  // task copies a single source-destination pair shallowly and queues
  // children for containers. Pre-filling destination containers with Null
  // placeholders gives stable addresses to queue child tasks against.
  // current_type is set only after the matching union member is fully
  // constructed, so a throw from the catch handler can safely destroy
  // whatever state is present
  struct CopyTask {
    const JSON *source;
    JSON *destination;
  };
  std::vector<CopyTask> tasks;
  tasks.reserve(16);
  tasks.push_back({.source = &other, .destination = this});

  try {
    while (!tasks.empty()) {
      const auto task{tasks.back()};
      tasks.pop_back();
      const JSON &source{*task.source};
      JSON &destination{*task.destination};
      switch (source.current_type_) {
        case Type::Null:
          break;
        case Type::Boolean:
          destination.data_boolean = source.data_boolean;
          destination.current_type_ = Type::Boolean;
          break;
        case Type::Integer:
          destination.data_integer = source.data_integer;
          destination.current_type_ = Type::Integer;
          break;
        case Type::Real:
          destination.data_real = source.data_real;
          destination.current_type_ = Type::Real;
          break;
        case Type::String:
          std::construct_at(&destination.data_string, source.data_string);
          destination.current_type_ = Type::String;
          break;
        case Type::Decimal:
          std::construct_at(&destination.data_decimal, source.data_decimal);
          destination.current_type_ = Type::Decimal;
          break;
        case Type::Array: {
          std::construct_at(&destination.data_array, Array{});
          destination.current_type_ = Type::Array;
          const auto &source_data{source.data_array.data_};
          auto &destination_data{destination.data_array.data_};
          destination_data.reserve(source_data.size());
          for (std::size_t index = 0; index < source_data.size(); ++index) {
            destination_data.emplace_back(nullptr);
          }
          for (std::size_t index = 0; index < source_data.size(); ++index) {
            tasks.push_back({.source = &source_data[index],
                             .destination = &destination_data[index]});
          }
          break;
        }
        case Type::Object: {
          std::construct_at(&destination.data_object, Object{});
          destination.current_type_ = Type::Object;
          const auto &source_data{source.data_object.data_};
          auto &destination_data{destination.data_object.data_};
          destination_data.reserve(source_data.size());
          for (const auto &entry : source_data) {
            destination_data.emplace_back(entry.first, JSON{nullptr},
                                          entry.hash);
          }
          for (std::size_t index = 0; index < source_data.size(); ++index) {
            tasks.push_back({.source = &source_data[index].second,
                             .destination = &destination_data[index].second});
          }
          break;
        }
      }
    }
  } catch (...) {
    // Tear down the partially-built tree before unwinding. *this's lifetime
    // never began, so ~JSON would not have run otherwise. The vector dtor
    // dispatches to each child's iterative ~JSON for any depth of subtree
    this->maybe_destruct_union();
    throw;
  }
}

JSON::JSON(JSON &&other) noexcept : current_type_{other.current_type_} {
  switch (other.current_type_) {
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
      std::construct_at(&this->data_string, std::move(other.data_string));
      other.current_type_ = Type::Null;
      break;
    case Type::Array:
      std::construct_at(&this->data_array, std::move(other.data_array));
      other.current_type_ = Type::Null;
      break;
    case Type::Object:
      std::construct_at(&this->data_object, std::move(other.data_object));
      other.current_type_ = Type::Null;
      break;
    case Type::Decimal:
      std::construct_at(&this->data_decimal, std::move(other.data_decimal));
      // Marking the source as empty means its destructor will never visit
      // the decimal member again, so end that member's lifetime here. The
      // moved-from state owns no heap coefficient, making this a no-op
      // branch rather than a deallocation
      other.data_decimal.~Decimal();
      other.current_type_ = Type::Null;
      break;
    default:
      break;
  }
}

auto JSON::operator=(const JSON &other) -> JSON & {
  if (this == &other) {
    return *this;
  }

  // Fast path for scalar sources: tear this value down iteratively, which is
  // safe for any depth, then assign the scalar directly. Each scalar is
  // buffered into a local first, because the source may be nested inside this
  // value, and tearing this value down would otherwise free the storage still
  // being read from
  switch (other.current_type_) {
    case Type::Null:
      this->~JSON();
      this->current_type_ = Type::Null;
      return *this;
    case Type::Boolean: {
      const auto value{other.data_boolean};
      this->~JSON();
      this->data_boolean = value;
      this->current_type_ = Type::Boolean;
      return *this;
    }
    case Type::Integer: {
      const auto value{other.data_integer};
      this->~JSON();
      this->data_integer = value;
      this->current_type_ = Type::Integer;
      return *this;
    }
    case Type::Real: {
      const auto value{other.data_real};
      this->~JSON();
      this->data_real = value;
      this->current_type_ = Type::Real;
      return *this;
    }
    case Type::String: {
      String value{other.data_string};
      this->~JSON();
      std::construct_at(&this->data_string, std::move(value));
      this->current_type_ = Type::String;
      return *this;
    }
    case Type::Decimal: {
      Decimal value{other.data_decimal};
      this->~JSON();
      std::construct_at(&this->data_decimal, std::move(value));
      this->current_type_ = Type::Decimal;
      return *this;
    }
    case Type::Array:
    case Type::Object:
      break;
  }

  // Container source: copy first (may throw) so this is unchanged on failure,
  // then destroy and shallow-move into place. Both the copy and the destroy
  // use the iterative pipeline and handle arbitrary depth
  JSON copy = other;
  this->~JSON();
  std::construct_at(this, std::move(copy));
  return *this;
}

auto JSON::operator=(JSON &&other) noexcept -> JSON & {
  if (this == &other) {
    return *this;
  }

  // Steal the source into a local before this value is torn down, because the
  // source may be nested inside this value, and tearing it down first would
  // free the storage still being moved from. Parentheses select the move
  // constructor rather than the list constructor
  JSON moved(std::move(other));
  this->~JSON();
  std::construct_at(this, std::move(moved));
  return *this;
}

JSON::~JSON() {
  // Drain only nested container children iteratively so deeply nested values
  // do not overflow the call stack. Scalar and empty children are left in
  // place to be destroyed by maybe_destruct_union, which is non-recursive
  // for non-container types. pending stays empty when no descendant has its
  // own container children, so containers full of scalars cost no heap
  // allocation. Allocation failures during destruction have no sensible
  // recovery, so terminate
  if (this->current_type_ == Type::Array ||
      this->current_type_ == Type::Object) {
    try {
      std::vector<JSON> pending;

      if (this->current_type_ == Type::Array) {
        for (auto &child : this->data_array.data_) {
          if (child.current_type_ == Type::Array ||
              child.current_type_ == Type::Object) {
            pending.push_back(std::move(child));
          }
        }
      } else {
        for (auto &entry : this->data_object.data_) {
          if (entry.second.current_type_ == Type::Array ||
              entry.second.current_type_ == Type::Object) {
            pending.push_back(std::move(entry.second));
          }
        }
      }

      while (!pending.empty()) {
        JSON node = std::move(pending.back());
        pending.pop_back();
        if (node.current_type_ == Type::Array) {
          for (auto &child : node.data_array.data_) {
            if (child.current_type_ == Type::Array ||
                child.current_type_ == Type::Object) {
              pending.push_back(std::move(child));
            }
          }
          node.data_array.~JSONArray();
          node.current_type_ = Type::Null;
        } else {
          for (auto &entry : node.data_object.data_) {
            if (entry.second.current_type_ == Type::Array ||
                entry.second.current_type_ == Type::Object) {
              pending.push_back(std::move(entry.second));
            }
          }
          node.data_object.~JSONObject();
          node.current_type_ = Type::Null;
        }
      }
    } catch (...) {
      std::terminate();
    }
  }

  this->maybe_destruct_union();
}

auto JSON::make_array() -> JSON { return JSON{Array{}}; }

auto JSON::make_array(std::initializer_list<JSON> values) -> JSON {
  JSON result{nullptr};
  std::construct_at(&result.data_array, values);
  result.current_type_ = Type::Array;
  return result;
}

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

// Ordering numbers of different representations by exact value may allocate an
// arbitrary precision expansion, so this comparison is not noexcept
auto JSON::operator<(const JSON &other) const -> bool {
  if (this->is_number() && other.is_number() &&
      this->current_type_ != other.current_type_) {
    return std::is_lt(cross_numeric_ordering(*this, other));
  }

  if (this->type() != other.type()) {
    return this->current_type_ < other.current_type_;
  }

  switch (this->type()) {
    case Type::Null:
      return false;
    case Type::Boolean:
      return static_cast<int>(this->to_boolean()) <
             static_cast<int>(other.to_boolean());
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

auto JSON::operator<=(const JSON &other) const -> bool {
  return *this < other || *this == other;
}

auto JSON::operator>(const JSON &other) const -> bool {
  return !(*this < other) && *this != other;
}

auto JSON::operator>=(const JSON &other) const -> bool {
  return *this > other || *this == other;
}

// Comparing numbers of different representations by exact value may allocate an
// arbitrary precision expansion, so this comparison is not noexcept
auto JSON::operator==(const JSON &other) const -> bool {
  if (this->is_number() && other.is_number() &&
      this->current_type_ != other.current_type_) {
    return std::is_eq(cross_numeric_ordering(*this, other));
  }

  if (this->current_type_ != other.current_type_) {
    return false;
  }

  switch (this->current_type_) {
    case Type::Boolean:
      return this->data_boolean == other.data_boolean;
    case Type::Integer:
      return this->data_integer == other.data_integer;
    case Type::Real:
      return this->data_real == other.data_real;
    case Type::Decimal:
      return this->data_decimal == other.data_decimal;
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
  }
  if (this->is_integer() && other.is_integer()) {
    const auto left{this->to_integer()};
    const auto right{other.to_integer()};
    // Promote to arbitrary precision when the sum would not fit, since signed
    // integer overflow is undefined behavior
    if ((right > 0 && left > std::numeric_limits<Integer>::max() - right) ||
        (right < 0 && left < std::numeric_limits<Integer>::min() - right)) {
      return JSON{Decimal{left} + Decimal{right}};
    }

    return JSON{left + right};
  }
  if (this->is_integer() && other.is_real()) {
    return JSON{this->as_real() + other.to_real()};
  }
  if (this->is_real() && other.is_integer()) {
    return JSON{this->to_real() + other.as_real()};
  }
  return JSON{this->to_real() + other.to_real()};
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
  }
  if (this->is_integer() && other.is_integer()) {
    const auto left{this->to_integer()};
    const auto right{other.to_integer()};
    // Promote to arbitrary precision when the difference would not fit, since
    // signed integer overflow is undefined behavior
    if ((right < 0 && left > std::numeric_limits<Integer>::max() + right) ||
        (right > 0 && left < std::numeric_limits<Integer>::min() + right)) {
      return JSON{Decimal{left} - Decimal{right}};
    }

    return JSON{left - right};
  }
  if (this->is_integer() && other.is_real()) {
    return JSON{this->as_real() - other.to_real()};
  }
  if (this->is_real() && other.is_integer()) {
    return JSON{this->to_real() - other.as_real()};
  }
  return JSON{this->to_real() - other.to_real()};
}

auto JSON::operator+=(const JSON &additive) -> JSON & {
  return *this = *this + additive;
}

auto JSON::operator-=(const JSON &substractive) -> JSON & {
  return *this = *this - substractive;
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

[[nodiscard]] auto JSON::to_stringstream() const
    -> std::basic_istringstream<Char, CharTraits, Allocator<Char>> {
  return std::basic_istringstream<Char, CharTraits, Allocator<Char>>{
      this->data_string};
}

[[nodiscard]] auto JSON::at_or(const String &key, const Object::hash_type hash,
                               const JSON &otherwise) const -> const JSON & {
  assert(this->is_object());
  const auto *const result{this->try_at(key, hash)};
  return (result != nullptr) ? *result : otherwise;
}

[[nodiscard]] auto JSON::at_or(const String &key, const JSON &otherwise) const
    -> const JSON & {
  assert(this->is_object());
  return this->at_or(key, this->data_object.hash(key), otherwise);
}

[[nodiscard]] auto JSON::estimated_byte_size() const -> std::uint64_t {
  // Of course, container have some overhead of their own
  // which we are not taking into account here, as its typically
  // implementation dependent. This function is just a rough estimate.
  if (this->is_object()) {
    return std::ranges::fold_left(
        this->as_object(), static_cast<std::uint64_t>(0),
        [](const std::uint64_t accumulator,
           const Object::value_type &pair) -> std::uint64_t {
          return accumulator + (pair.first.size() * sizeof(Char)) +
                 pair.second.estimated_byte_size();
        });
  }
  if (this->is_array()) {
    return std::ranges::fold_left(
        this->as_array(), static_cast<std::uint64_t>(0),
        [](const std::uint64_t accumulator, const JSON &item) -> std::uint64_t {
          return accumulator + item.estimated_byte_size();
        });
  }
  if (this->is_string()) {
    // Keep in mind that standard strings might reserve more
    // space than what it is actually used by the string
    return this->byte_size() * sizeof(Char);
  }
  if (this->is_integer()) {
    return sizeof(Integer);
  }
  if (this->is_real()) {
    return sizeof(Real);
  }
  if (this->is_boolean()) {
    return sizeof(bool);
  } // The size of the union
  return 8;
}

[[nodiscard]] auto JSON::fast_hash() const -> std::uint64_t {
  switch (this->current_type_) {
    case Type::Null:
      return 2;
    case Type::Boolean:
      return this->to_boolean() ? 1 : 0;
    case Type::Integer:
      return 4 + (static_cast<std::uint64_t>(this->to_integer()) % 256);
    case Type::Real: {
      // A number that equals an in-range integer must hash like that integer,
      // otherwise equal values across representations would hash differently.
      // The integer round-trip avoids a std::modf library call
      const auto value{this->to_real()};
      if (value >= static_cast<Real>(std::numeric_limits<Integer>::min()) &&
          value < static_cast<Real>(std::numeric_limits<Integer>::max())) {
        const auto truncated{static_cast<Integer>(value)};
        if (static_cast<Real>(truncated) == value) {
          return 4 + (static_cast<std::uint64_t>(truncated) % 256);
        }
      }

      return 5;
    }
    case Type::String:
      return 3 + this->byte_size();
    case Type::Array:
      return std::ranges::fold_left(this->as_array(),
                                    static_cast<std::uint64_t>(6),
                                    [](const std::uint64_t accumulator,
                                       const JSON &item) -> std::uint64_t {
                                      return accumulator + 1 + item.fast_hash();
                                    });
    case Type::Object:
      return std::ranges::fold_left(
          this->as_object(), static_cast<std::uint64_t>(7),
          [](const std::uint64_t accumulator,
             const Object::value_type &pair) -> std::uint64_t {
            return accumulator + 1 + pair.first.size() +
                   pair.second.fast_hash();
          });
    case Type::Decimal: {
      const auto &decimal{this->to_decimal()};
      if (decimal.is_integral()) {
        const auto integral{decimal.to_integral()};
        if (integral.is_int64()) {
          return 4 + (static_cast<std::uint64_t>(integral.to_int64()) % 256);
        }
      }

      return 5;
    }
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

  // Reading an integer operand as a real rounds it once it no longer fits the
  // significand, which both misses and invents divisors, so past that point
  // each operand is converted from the storage that holds it exactly
  if (this->is_integer() && divisor.is_real() &&
      !integer_fits_real(this->to_integer())) {
    const Decimal dividend_decimal{this->to_integer()};
    return dividend_decimal.divisible_by(
        Decimal::strict_from(divisor.to_real()));
  }

  if (this->is_real() && divisor.is_integer() &&
      !integer_fits_real(divisor.to_integer())) {
    const Decimal divisor_decimal{divisor.to_integer()};
    return Decimal::strict_from(this->to_real()).divisible_by(divisor_decimal);
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
    if (!std::isinf(division) && !std::isnan(division) &&
        std::modf(division, &integral) == 0.0) {
      return true;
    }

    return Decimal::strict_from(dividend_value)
        .divisible_by(Decimal::strict_from(divisor_value));
  }

  if (this->is_decimal() && divisor.is_decimal()) {
    return this->to_decimal().divisible_by(divisor.to_decimal());
  }

  if (this->is_decimal()) {
    if (divisor.is_integer()) {
      const Decimal divisor_decimal{divisor.to_integer()};
      return this->to_decimal().divisible_by(divisor_decimal);
    }

    return this->to_decimal().divisible_by(
        Decimal::strict_from(divisor.to_real()));
  }

  if (this->is_integer()) {
    const Decimal dividend_decimal{this->to_integer()};
    return dividend_decimal.divisible_by(divisor.to_decimal());
  }

  return Decimal::strict_from(this->to_real())
      .divisible_by(divisor.to_decimal());
}

[[nodiscard]] auto
JSON::defines_any(std::initializer_list<JSON::String> keys) const -> bool {
  return this->defines_any(keys.begin(), keys.end());
}

[[nodiscard]] auto JSON::contains(const JSON &element) const -> bool {
  assert(this->is_array());
  return std::ranges::contains(this->as_array(), element);
}

[[nodiscard]] auto JSON::contains(const JSON::StringView element) const
    -> bool {
  assert(this->is_array());
  for (const auto &item : this->as_array()) {
    if (item.is_string() && item.to_string() == element) {
      return true;
    }
  }

  return false;
}

[[nodiscard]] auto JSON::includes(const JSON::String &input) const -> bool {
  assert(this->is_string());
  return this->to_string().contains(input);
}

[[nodiscard]] auto JSON::includes(const JSON::String::value_type input) const
    -> bool {
  assert(this->is_string());
  return this->to_string().contains(input);
}

[[nodiscard]] auto JSON::unique() const -> bool {
  assert(this->is_array());
  const auto &items{this->data_array.data_};
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

  // Two items can only be equal when their hashes are, so the search for a
  // repeated item is a search for a repeated hash first, and only the positions
  // that survive it are worth comparing in full. Comparing in full and skipping
  // ahead are kept in separate loops so that an array whose items nearly all
  // share one hash never reaches the skip, and pays exactly what it would have
  // paid for a plain scan
  const auto *const hashes{cache.data()};
  for (std::size_t index = 0; index + 1 < size; index++) {
    const auto needle{hashes[index]};
    auto subindex{index + 1};
    while (subindex < size) {
      while (subindex < size && hashes[subindex] == needle) {
        if (items[index] == items[subindex]) {
          return false;
        }

        subindex += 1;
      }

      if (subindex >= size) {
        break;
      }

      subindex += scan_hashes(hashes + subindex, size - subindex, needle);
    }
  }

  return true;
}

[[nodiscard]] auto JSON::unique_keys() const -> bool {
  assert(this->is_object());
  const auto &entries{this->data_object.data_};
  const auto size{entries.size()};

  // Objects of 0 or 1 member have unique keys by definition
  if (size <= 1) {
    return true;
  }

  for (std::size_t index = 0; index < size; index++) {
    for (std::size_t subindex = index + 1; subindex < size; subindex++) {
      if (entries[subindex].key_equals(entries[index].first,
                                       entries[index].hash)) {
        return false;
      }
    }
  }

  return true;
}

auto JSON::push_back(const JSON &value) -> void {
  assert(this->is_array());
  this->data_array.data_.push_back(value);
}

auto JSON::push_back(JSON &&value) -> void {
  assert(this->is_array());
  this->data_array.data_.push_back(std::move(value));
}

auto JSON::push_back_if_unique(const JSON &value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  assert(this->is_array());
  auto &array_data{this->as_array().data_};
  if (!std::ranges::contains(array_data, value)) {
    array_data.push_back(value);
    return {array_data.back(), true};
  }
  return {*std::ranges::find(array_data, value), false};
}

auto JSON::push_back_if_unique(JSON &&value)
    -> std::pair<std::reference_wrapper<const JSON>, bool> {
  assert(this->is_array());
  auto &array_data{this->as_array().data_};
  if (!std::ranges::contains(array_data, value)) {
    array_data.push_back(std::move(value));
    return {array_data.back(), true};
  }
  return {*std::ranges::find(array_data, value), false};
}

auto JSON::assign(const JSON::String &key, const JSON &value) -> void {
  assert(this->is_object());
  this->data_object.emplace(key, value);
}

auto JSON::assign(const JSON::String &key, JSON &&value) -> void {
  assert(this->is_object());
  this->data_object.emplace(key, std::move(value));
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

auto JSON::assign_assume_new(JSON::String &&key, JSON &&value,
                             Object::hash_type hash) -> JSON & {
  assert(this->is_object());
  return this->data_object.emplace_assume_new(std::move(key), std::move(value),
                                              hash);
}

auto JSON::erase(const JSON::String &key) -> Object::size_type {
  assert(this->is_object());
  return this->data_object.erase(key);
}

auto JSON::erase_keys(std::initializer_list<JSON::String> keys) -> void {
  this->erase_keys(keys.begin(), keys.end());
}

auto JSON::erase(JSON::Array::const_iterator position)
    -> JSON::Array::iterator {
  assert(this->is_array());
  return this->data_array.data_.erase(position);
}

auto JSON::erase(JSON::Array::const_iterator first,
                 JSON::Array::const_iterator last) -> JSON::Array::iterator {
  assert(this->is_array());
  return this->data_array.data_.erase(first, last);
}

auto JSON::erase_if(const std::function<bool(const JSON &)> &predicate)
    -> void {
  assert(this->is_array());
  std::erase_if(this->data_array.data_, predicate);
}

auto JSON::clear() -> void {
  if (this->is_object()) {
    this->data_object.clear();
  } else {
    this->data_array.data_.clear();
  }
}

auto JSON::clear_except(std::initializer_list<JSON::String> keys) -> void {
  this->clear_except(keys.begin(), keys.end());
}

auto JSON::merge(const JSON::Object &other) -> void {
  assert(this->is_object());
  // When the source is this object's own container, the insertions below would
  // reallocate the very storage being iterated, so it is snapshotted first
  if (&other == &this->data_object) {
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    const JSON::Object snapshot{other};
    this->merge(snapshot);
    return;
  }

  for (const auto &pair : other) {
    auto *const maybe_key{this->try_at(pair.first, pair.hash)};
    if ((maybe_key != nullptr) && maybe_key->is_object() &&
        pair.second.is_object()) {
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

auto JSON::rename(const JSON::String &key, JSON::String &&target) -> void {
  assert(this->is_object());
  auto &object{this->data_object};
  object.rename(key, object.hash(key), std::move(target), object.hash(target));
}

auto JSON::into(const JSON &other) -> void { this->operator=(other); }

auto JSON::into(JSON &&other) noexcept -> void {
  this->operator=(std::move(other));
}

auto JSON::into_array() -> void { this->into(JSON::make_array()); }

auto JSON::into_object() -> void { this->into(JSON::make_object()); }

auto JSON::maybe_destruct_union() -> void {
  switch (this->current_type_) {
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
      this->data_decimal.~Decimal();
      break;
    default:
      break;
  }

  this->current_type_ = Type::Null;
}

} // namespace sourcemeta::core
