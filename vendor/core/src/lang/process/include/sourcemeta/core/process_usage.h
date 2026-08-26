#ifndef SOURCEMETA_CORE_PROCESS_USAGE_H_
#define SOURCEMETA_CORE_PROCESS_USAGE_H_

#ifndef SOURCEMETA_CORE_PROCESS_EXPORT
#include <sourcemeta/core/process_export.h>
#endif

#include <chrono>   // std::chrono::nanoseconds, std::chrono::system_clock
#include <cstdint>  // std::uint64_t
#include <optional> // std::optional

namespace sourcemeta::core {

/// @ingroup process
/// What the running process has consumed so far.
///
/// Every member carries no value where the platform cannot cheaply answer, so
/// that a caller reports nothing rather than a zero it cannot tell apart from
/// a measurement.
struct ProcessUsage {
  /// Combined user and system processor time
  std::optional<std::chrono::nanoseconds> cpu_time{std::nullopt};
  /// Physical memory currently held, in bytes
  std::optional<std::uint64_t> resident_bytes{std::nullopt};
  /// Address space currently mapped, in bytes
  std::optional<std::uint64_t> virtual_bytes{std::nullopt};
};

/// @ingroup process
/// What the running process currently holds open, and how much it may.
///
/// Every member carries no value where the platform cannot cheaply answer, so
/// that a caller reports nothing rather than a zero it cannot tell apart from
/// a measurement.
struct ProcessDescriptors {
  /// Open file descriptors, or open handles on platforms that count those
  std::optional<std::uint64_t> open{std::nullopt};
  /// The ceiling the platform enforces, where there is one and it is finite
  std::optional<std::uint64_t> maximum{std::nullopt};
};

/// @ingroup process
///
/// Read what the running process has consumed so far. For example:
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto usage{sourcemeta::core::process_usage()};
/// assert(usage.cpu_time.value() >= std::chrono::nanoseconds::zero());
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto process_usage() noexcept -> ProcessUsage;

/// @ingroup process
///
/// Read what the running process currently holds open. This costs a directory
/// scan on some platforms. For example:
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <cassert>
///
/// const auto descriptors{sourcemeta::core::process_descriptors()};
/// assert(descriptors.open.value() > 0);
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto process_descriptors() noexcept -> ProcessDescriptors;

/// @ingroup process
///
/// Read when the running process began. The answer does not change. For
/// example:
///
/// ```cpp
/// #include <sourcemeta/core/process.h>
/// #include <chrono>
/// #include <cassert>
///
/// const auto started{sourcemeta::core::process_start_time()};
/// assert(started.value() <= std::chrono::system_clock::now());
/// ```
SOURCEMETA_CORE_PROCESS_EXPORT
auto process_start_time() noexcept
    -> std::optional<std::chrono::system_clock::time_point>;

} // namespace sourcemeta::core

#endif
