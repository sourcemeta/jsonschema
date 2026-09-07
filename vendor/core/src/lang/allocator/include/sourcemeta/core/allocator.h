#ifndef SOURCEMETA_CORE_ALLOCATOR_H_
#define SOURCEMETA_CORE_ALLOCATOR_H_

#ifndef SOURCEMETA_CORE_ALLOCATOR_EXPORT
#include <sourcemeta/core/allocator_export.h>
#endif

#include <cstdint> // std::uint8_t

/// @defgroup allocator Allocator
/// @brief The allocator that every program in this project runs on.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/core/allocator.h>
/// ```
///
/// Linking this library into a program is what puts its allocator in place, as
/// the standard allocation entry points it defines are the ones the program
/// already calls. No other library in this project links it, so a program that
/// does not ask for it keeps the allocator its platform ships with.

namespace sourcemeta::core {

/// @ingroup allocator
/// The allocators that a program can be built against.
enum class Allocator : std::uint8_t {
  /// The allocator this project vendors and links into its own programs
  Mimalloc,
  /// The allocator the platform ships with
  System
};

/// @ingroup allocator
///
/// Report the allocator this program was linked against. For example:
///
/// ```cpp
/// #include <sourcemeta/core/allocator.h>
/// #include <iostream>
///
/// auto main() -> int {
///   const auto selected{sourcemeta::core::allocator()};
///   if (selected == sourcemeta::core::Allocator::Mimalloc) {
///     std::cerr << "This program brings its own allocator\n";
///   }
/// }
/// ```
SOURCEMETA_CORE_ALLOCATOR_EXPORT
auto allocator() -> Allocator;

} // namespace sourcemeta::core

#endif
