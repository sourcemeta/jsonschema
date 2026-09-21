#ifndef SOURCEMETA_CORE_BENCHMARK_H_
#define SOURCEMETA_CORE_BENCHMARK_H_

#ifndef SOURCEMETA_CORE_BENCHMARK_EXPORT
#include <sourcemeta/core/benchmark_export.h>
#endif

#include <sourcemeta/core/preprocessor.h>

#include <chrono>      // std::chrono::nanoseconds, std::chrono::steady_clock
#include <cstdint>     // std::uint64_t
#include <functional>  // std::function
#include <optional>    // std::optional
#include <string_view> // std::string_view
#include <type_traits> // std::is_trivially_copyable_v

#if defined(_MSC_VER)
#include <intrin.h> // _ReadWriteBarrier
#endif

/// @defgroup benchmark Benchmark
/// @brief A minimal microbenchmarking framework
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/benchmark.h>
/// ```

namespace sourcemeta::core {

// Exporting symbols that depends on the standard C++ library is considered
// safe.
// https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4275?view=msvc-170&redirectedfrom=MSDN
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

/// @ingroup benchmark
///
/// The state of one benchmark run, i.e. how many iterations to perform and the
/// time they took. A benchmark body obtains it from the registration macro and
/// consumes it by iterating over it.
class SOURCEMETA_CORE_BENCHMARK_EXPORT BenchmarkState {
public:
  /// Prepare a run of the given number of iterations.
  explicit BenchmarkState(const std::uint64_t iterations);

  /// The value a loop iteration yields. It carries nothing, as the loop exists
  /// only to repeat its body a known number of times.
  struct [[maybe_unused]] Value {};

  /// Walks the remaining iterations of a run. Every member is defined here on
  /// purpose, i.e. the loop must cost a decrement and a compare, not a call
  /// into another translation unit, and a nested class of an exported class is
  /// not itself exported on Windows.
  class Iterator {
  public:
    /// Construct the sentinel that the loop compares against.
    Iterator() : remaining_{0}, parent_{nullptr} {}

    /// Construct the position that walks the given run.
    explicit Iterator(BenchmarkState *const parent)
        : remaining_{parent->iterations_}, parent_{parent} {}

    /// Yield the current iteration.
    SOURCEMETA_FORCEINLINE auto operator*() const -> Value { return {}; }

    /// Advance to the next iteration.
    SOURCEMETA_FORCEINLINE auto operator++() -> Iterator & {
      this->remaining_ -= 1;
      return *this;
    }

    /// Report whether the run has iterations left, stopping the clock when it
    /// does not.
    SOURCEMETA_FORCEINLINE auto operator!=(const Iterator &other) const
        -> bool {
      if (this->remaining_ != other.remaining_) [[likely]] {
        return true;
      }

      // The sentinel carries no run to stop, and a comparison repeated after
      // the loop has already ended must not restate the elapsed time
      if (this->parent_ != nullptr) {
        this->parent_->finish();
      }

      return false;
    }

  private:
    std::uint64_t remaining_;
    BenchmarkState *parent_;
  };

  /// The position the loop starts from.
  SOURCEMETA_FORCEINLINE auto begin() -> Iterator { return Iterator{this}; }

  /// The sentinel the loop stops at. Obtaining it starts the clock, so that as
  /// little as possible of the caller's setup falls inside the measurement.
  SOURCEMETA_FORCEINLINE auto end() -> Iterator {
    this->start();
    return {};
  }

  /// How many iterations this run was asked to perform.
  [[nodiscard]] auto iterations() const -> std::uint64_t;
  /// Whether the body actually iterated.
  [[nodiscard]] auto measured() const -> bool;
  /// How long the iterations took on the clock.
  [[nodiscard]] auto real_time() const -> std::chrono::nanoseconds;
  /// How much processor time the iterations consumed, where the platform can
  /// answer.
  [[nodiscard]] auto cpu_time() const
      -> std::optional<std::chrono::nanoseconds>;

private:
  auto start() -> void;
  auto finish() -> void;

  std::uint64_t iterations_;
  bool started_{false};
  bool finished_{false};
  std::chrono::steady_clock::time_point real_start_{};
  std::chrono::nanoseconds real_elapsed_{0};
  std::optional<std::chrono::nanoseconds> cpu_start_{std::nullopt};
  std::optional<std::chrono::nanoseconds> cpu_elapsed_{std::nullopt};
};

// Only a platform without inline assembly needs an opaque sink to make a value
// escape, so it is not declared anywhere else. Defining it unconditionally
// would leave a function that no build using assembly can ever reach
#if !defined(__clang__) && !defined(__GNUC__)

/// @ingroup benchmark
///
/// Consume a pointer without looking at it. Only the platforms that offer no
/// inline assembly rely on this.
SOURCEMETA_CORE_BENCHMARK_EXPORT
auto benchmark_use_char_pointer(const volatile char *pointer) -> void;

#endif

// Inline assembly is the only portable way to tell a compiler that a value has
// escaped, and an empty instruction sequence is what makes the barrier free.
// Without it the optimiser deletes the very work under measurement, silently,
// and the benchmark reports a result that was never computed.
//
// The constraint strings below are those of Google Benchmark, which is licensed
// under Apache-2.0, at
// https://github.com/google/benchmark/blob/v1.8.5/include/benchmark/benchmark.h
// They are reproduced rather than reinvented because they are the most
// exercised form of this barrier in the wild, and because the details are not
// guessable. Clang wants the register alternative first while GCC wants memory
// first, and GCC additionally copies the whole argument unless a value that is
// neither trivially copyable nor register sized is constrained to memory alone,
// per https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105519
//
// A second library arrived at the same code independently, i.e. nanobench
// records that it moved off Facebook's folly after hitting compilation problems
// and settled on these exact constraints
//
// NOLINTBEGIN(hicpp-no-assembler)

/// @ingroup benchmark
///
/// Keep a value the compiler would otherwise discard. For example:
///
/// ```cpp
/// #include <sourcemeta/core/benchmark.h>
///
/// BENCHMARK(Addition) {
///   for (auto iteration : state) {
///     auto result{1 + 1};
///     sourcemeta::core::benchmark_do_not_optimize(result);
///   }
/// }
/// ```
template <typename Type>
SOURCEMETA_FORCEINLINE inline auto benchmark_do_not_optimize(Type &value)
    -> void {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#elif defined(__GNUC__)
  if constexpr (std::is_trivially_copyable_v<Type> &&
                sizeof(Type) <= sizeof(void *)) {
    asm volatile("" : "+m,r"(value) : : "memory");
  } else {
    asm volatile("" : "+m"(value) : : "memory");
  }
#elif defined(_MSC_VER)
  benchmark_use_char_pointer(&reinterpret_cast<const volatile char &>(value));
  _ReadWriteBarrier();
#else
  benchmark_use_char_pointer(&reinterpret_cast<const volatile char &>(value));
#endif
}

/// @ingroup benchmark
///
/// Keep a value the compiler would otherwise discard, where the caller holds it
/// by constant reference or produced it as a temporary. For example:
///
/// ```cpp
/// #include <sourcemeta/core/benchmark.h>
///
/// BENCHMARK(Addition) {
///   const auto left{1};
///   for (auto iteration : state) {
///     sourcemeta::core::benchmark_do_not_optimize(left + 1);
///   }
/// }
/// ```
template <typename Type>
SOURCEMETA_FORCEINLINE inline auto benchmark_do_not_optimize(const Type &value)
    -> void {
#if defined(__clang__)
  asm volatile("" : : "r,m"(value) : "memory");
#elif defined(__GNUC__)
  if constexpr (std::is_trivially_copyable_v<Type> &&
                sizeof(Type) <= sizeof(void *)) {
    asm volatile("" : : "r,m"(value) : "memory");
  } else {
    asm volatile("" : : "m"(value) : "memory");
  }
#elif defined(_MSC_VER)
  benchmark_use_char_pointer(&reinterpret_cast<const volatile char &>(value));
  _ReadWriteBarrier();
#else
  benchmark_use_char_pointer(&reinterpret_cast<const volatile char &>(value));
#endif
}

// NOLINTEND(hicpp-no-assembler)

/// @ingroup benchmark
///
/// Register a benchmark by name with a callable body. Returns a dummy value so
/// it can be used to initialise a namespace-scope variable.
SOURCEMETA_CORE_BENCHMARK_EXPORT
auto benchmark_register(std::string_view name, std::string_view file, int line,
                        std::function<void(BenchmarkState &)> body) -> int;

/// @ingroup benchmark
///
/// Run every registered benchmark, optionally filtered, and return a process
/// exit code.
SOURCEMETA_CORE_BENCHMARK_EXPORT
auto benchmark_run(int argc, char **argv) -> int;

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace sourcemeta::core

// The registration symbol is a namespace-scope object whose initializer runs
// the registry call, which no static initialization check can prove
// non-throwing
// NOLINTBEGIN(cert-err58-cpp,bugprone-throwing-static-initialization)

// The body takes its state as a parameter that the caller never spells, which
// is what keeps the reported label out of the identifier namespace and off
// every naming rule that would otherwise apply to it
#define SOURCEMETA_CORE_BENCHMARK_REGISTER(name)                               \
  static auto sourcemeta_benchmark_body_##name(                                \
      ::sourcemeta::core::BenchmarkState &)                                    \
      ->void;                                                                  \
  [[maybe_unused]] static const int sourcemeta_benchmark_registration_##name = \
      ::sourcemeta::core::benchmark_register(                                  \
          #name, __FILE__, __LINE__, &sourcemeta_benchmark_body_##name);       \
  static auto sourcemeta_benchmark_body_##name(                                \
      ::sourcemeta::core::BenchmarkState &state)                               \
      ->void

#define BENCHMARK(name) SOURCEMETA_CORE_BENCHMARK_REGISTER(name)
// NOLINTEND(cert-err58-cpp,bugprone-throwing-static-initialization)

#endif
