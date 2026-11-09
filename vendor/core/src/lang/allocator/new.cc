#include <mimalloc.h>

#include "entrypoint.h"

#include <cstddef> // std::size_t
#include <new> // std::align_val_t, std::nothrow_t, std::bad_alloc, std::get_new_handler

namespace {

// What this language asks of a failed allocation, which the allocator behind
// these entry points cannot do for us, as it is compiled as the language that
// has no way of reporting a failure other than ending the program. See
// [basic.stc.dynamic.allocation] and [new.delete.single]
//
// This stands apart from the entry points that reach it, and stays apart, so
// that asking for memory and getting it remains a request and a test of the
// answer, with the asking again and the giving up out of its way
__attribute__((noinline)) auto ask_again_or_report(const std::size_t size,
                                                   const std::size_t alignment)
    -> void * {
  while (true) {
    const auto handler{std::get_new_handler()};
    if (handler == nullptr) {
      throw std::bad_alloc{};
    }

    handler();

    void *pointer{alignment == 0 ? mi_malloc(size)
                                 : mi_malloc_aligned(size, alignment)};
    if (pointer != nullptr) {
      return pointer;
    }
  }
}

// The reporting forms are specified in terms of the ones that do report, down
// to answering with nothing where those would have thrown. Keeping the catch
// here rather than one step closer to the entry points is what lets the entry
// points that answer with nothing stay as plain as the ones that do report
__attribute__((noinline)) auto
ask_again_or_nothing(const std::size_t size,
                     const std::size_t alignment) noexcept -> void * {
  try {
    return ask_again_or_report(size, alignment);
  } catch (...) {
    return nullptr;
  }
}

auto allocate_or_report(const std::size_t size, const std::size_t alignment)
    -> void * {
  void *pointer{alignment == 0 ? mi_malloc(size)
                               : mi_malloc_aligned(size, alignment)};
  if (pointer == nullptr) [[unlikely]] {
    return ask_again_or_report(size, alignment);
  }

  return pointer;
}

auto allocate_or_nothing(const std::size_t size,
                         const std::size_t alignment) noexcept -> void * {
  void *pointer{alignment == 0 ? mi_malloc(size)
                               : mi_malloc_aligned(size, alignment)};
  if (pointer == nullptr) [[unlikely]] {
    return ask_again_or_nothing(size, alignment);
  }

  return pointer;
}

} // namespace

// Under the redirecting arrangement the platform declines to redirect a name
// that this library also defines, so these eight answer to names of their own
// and the redirection puts them in the platform's place. The twelve that
// follow it are the ones the platform does not export, which leaves defining
// them the only way they exist
#ifdef SOURCEMETA_CORE_ALLOCATOR_REDIRECT

extern "C" {

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto sourcemeta_core_allocator_new(std::size_t size) -> void * {
  return allocate_or_report(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto sourcemeta_core_allocator_new_nothrow(std::size_t size) noexcept
    -> void * {
  return allocate_or_nothing(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto sourcemeta_core_allocator_delete(void *pointer) noexcept -> void {
  mi_free(pointer);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto sourcemeta_core_allocator_delete_size(void *pointer,
                                           std::size_t size) noexcept -> void {
  mi_free_size(pointer, size);
}

} // extern "C"

#else

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new(std::size_t size) -> void * {
  return allocate_or_report(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new[](std::size_t size) -> void * {
  return allocate_or_report(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new(std::size_t size, const std::nothrow_t &) noexcept -> void * {
  return allocate_or_nothing(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new[](std::size_t size, const std::nothrow_t &) noexcept
    -> void * {
  return allocate_or_nothing(size, 0);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer) noexcept -> void { mi_free(pointer); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer) noexcept -> void { mi_free(pointer); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer, std::size_t size) noexcept -> void {
  mi_free_size(pointer, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer, std::size_t size) noexcept -> void {
  mi_free_size(pointer, size);
}

#endif

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new(std::size_t size, std::align_val_t alignment) -> void * {
  return allocate_or_report(size, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new[](std::size_t size, std::align_val_t alignment) -> void * {
  return allocate_or_report(size, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new(std::size_t size, std::align_val_t alignment,
                  const std::nothrow_t &) noexcept -> void * {
  return allocate_or_nothing(size, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator new[](std::size_t size, std::align_val_t alignment,
                    const std::nothrow_t &) noexcept -> void * {
  return allocate_or_nothing(size, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer, const std::nothrow_t &) noexcept -> void {
  mi_free(pointer);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer, const std::nothrow_t &) noexcept -> void {
  mi_free(pointer);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer, std::align_val_t alignment) noexcept
    -> void {
  mi_free_aligned(pointer, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer, std::align_val_t alignment) noexcept
    -> void {
  mi_free_aligned(pointer, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer, std::align_val_t alignment,
                     const std::nothrow_t &) noexcept -> void {
  mi_free_aligned(pointer, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer, std::align_val_t alignment,
                       const std::nothrow_t &) noexcept -> void {
  mi_free_aligned(pointer, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete(void *pointer, std::size_t size,
                     std::align_val_t alignment) noexcept -> void {
  mi_free_size_aligned(pointer, size, static_cast<std::size_t>(alignment));
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
auto operator delete[](void *pointer, std::size_t size,
                       std::align_val_t alignment) noexcept -> void {
  mi_free_size_aligned(pointer, size, static_cast<std::size_t>(alignment));
}
