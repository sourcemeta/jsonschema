// This project stays within ISO C, under which the platform declares only what
// the standard asks of it, while most of the entry points below are extensions
// that it hides until a build says it wants them
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <mimalloc.h>

#include "entrypoint.h"

#include <stdlib.h> // size_t, malloc, calloc, realloc, free, posix_memalign, aligned_alloc, valloc, reallocarray
#include <string.h> // strdup, strndup

#if defined(__APPLE__)
#include <AvailabilityMacros.h> // MAC_OS_X_VERSION_10_7, MAC_OS_X_VERSION_MAX_ALLOWED
#include <malloc/malloc.h> // malloc_size, malloc_good_size
#else
#include <malloc.h> // memalign, pvalloc, malloc_usable_size
#endif

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(malloc)(size_t size) {
  return mi_malloc(size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(calloc)(size_t count, size_t size) {
  return mi_calloc(count, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(realloc)(void *pointer, size_t size) {
  return mi_realloc(pointer, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void SOURCEMETA_CORE_ALLOCATOR_REPLACES(free)(void *pointer) {
  mi_free(pointer);
}

// Neither of the two below has to go through this allocator, but on some
// platforms they reach for something more primitive than the general entry
// point, and on others the platform makes them macros that cannot be replaced
#if !defined(strdup)
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
char *SOURCEMETA_CORE_ALLOCATOR_REPLACES(strdup)(const char *string) {
  return mi_strdup(string);
}
#endif

#if !defined(strndup) &&                                                       \
    (!defined(__APPLE__) ||                                                    \
     (defined(MAC_OS_X_VERSION_10_7) &&                                        \
      MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7))
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
char *SOURCEMETA_CORE_ALLOCATOR_REPLACES(strndup)(const char *string,
                                                  size_t length) {
  return mi_strndup(string, length);
}
#endif

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(reallocf)(void *pointer, size_t size) {
  return mi_reallocf(pointer, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
size_t SOURCEMETA_CORE_ALLOCATOR_REPLACES(malloc_size)(const void *pointer) {
  return mi_malloc_size(pointer);
}

#if !defined(__ANDROID__)
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
size_t malloc_usable_size(void *pointer) {
  return mi_malloc_usable_size(pointer);
}
#else
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
size_t malloc_usable_size(const void *pointer) {
  return mi_malloc_usable_size(pointer);
}
#endif

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(valloc)(size_t size) {
  return mi_valloc(size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void SOURCEMETA_CORE_ALLOCATOR_REPLACES(vfree)(void *pointer) {
  mi_free(pointer);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
size_t SOURCEMETA_CORE_ALLOCATOR_REPLACES(malloc_good_size)(size_t size) {
  return mi_malloc_good_size(size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
int SOURCEMETA_CORE_ALLOCATOR_REPLACES(posix_memalign)(void **pointer,
                                                       size_t alignment,
                                                       size_t size) {
  return mi_posix_memalign(pointer, alignment, size);
}

// A certain platform declares this one as an inline function of its own, which
// no definition here can replace. That inline function allocates through one of
// the entry points above, so it ends up here anyway
#if !defined(__GLIBC__) || __USE_ISOC11
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *SOURCEMETA_CORE_ALLOCATOR_REPLACES(aligned_alloc)(size_t alignment,
                                                        size_t size) {
  return mi_aligned_alloc(alignment, size);
}
#endif

// This one allocates on its caller's behalf and leaves the caller to release
// the result, so under the redirecting arrangement it has to come from here
// too. Where the platform's own names are replaced by defining them, the
// platform's version already allocates through the entry points above
#ifdef SOURCEMETA_CORE_ALLOCATOR_REDIRECT
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
char *SOURCEMETA_CORE_ALLOCATOR_REPLACES(realpath)(const char *path,
                                                   char *resolved) {
  return mi_realpath(path, resolved);
}
#endif

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void cfree(void *pointer) { mi_free(pointer); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *pvalloc(size_t size) { return mi_pvalloc(size); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *memalign(size_t alignment, size_t size) {
  return mi_memalign(alignment, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *_aligned_malloc(size_t size, size_t alignment) {
  return mi_malloc_aligned(size, alignment);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *reallocarray(void *pointer, size_t count, size_t size) {
  return mi_reallocarray(pointer, count, size);
}

// Some platforms ship one of their own, so defer to theirs where it exists
SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
__attribute__((weak)) int reallocarr(void *pointer, size_t count, size_t size) {
  return mi_reallocarr(pointer, count, size);
}

// The platform's own general purpose allocator reaches its implementation
// through the names below rather than through the entry points above, and
// parts of it keep doing so no matter what those entry points say
#if defined(__linux__)

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_malloc(size_t size) { return mi_malloc(size); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_calloc(size_t count, size_t size) {
  return mi_calloc(count, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_realloc(void *pointer, size_t size) {
  return mi_realloc(pointer, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void __libc_free(void *pointer) { mi_free(pointer); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void __libc_cfree(void *pointer) { mi_free(pointer); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_valloc(size_t size) { return mi_valloc(size); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_pvalloc(size_t size) { return mi_pvalloc(size); }

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
void *__libc_memalign(size_t alignment, size_t size) {
  return mi_memalign(alignment, size);
}

SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT
int __posix_memalign(void **pointer, size_t alignment, size_t size) {
  return mi_posix_memalign(pointer, alignment, size);
}

#endif

// How this platform is asked to put the entry points above in place of its own.
// The four that this language cannot name are the allocation and deallocation
// forms of the other one, which the platform binds by their encoded names
#ifdef SOURCEMETA_CORE_ALLOCATOR_REDIRECT

void *sourcemeta_core_allocator_new(size_t size);
void *sourcemeta_core_allocator_new_nothrow(size_t size);
void sourcemeta_core_allocator_delete(void *pointer);
void sourcemeta_core_allocator_delete_size(void *pointer, size_t size);

void vfree(void *pointer);
void *_Znwm(size_t size);
void *_Znam(size_t size);
void *_ZnwmRKSt9nothrow_t(size_t size, void *tag);
void *_ZnamRKSt9nothrow_t(size_t size, void *tag);
void _ZdlPv(void *pointer);
void _ZdaPv(void *pointer);
void _ZdlPvm(void *pointer, size_t size);
void _ZdaPvm(void *pointer, size_t size);

struct sourcemeta_core_allocator_redirection {
  const void *replacement;
  const void *target;
};

#define SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(from, to)                        \
  {(const void *)&to, (const void *)&from}

#define SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(name)                         \
  SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(name, sourcemeta_core_allocator_##name)

__attribute__((used)) static const struct sourcemeta_core_allocator_redirection
    SOURCEMETA_CORE_ALLOCATOR_REDIRECTIONS[]
    __attribute__((section("__DATA,__interpose"))) = {
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(malloc),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(calloc),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(realloc),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(free),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(strdup),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(strndup),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(realpath),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(posix_memalign),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(reallocf),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(valloc),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(vfree),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(malloc_size),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(malloc_good_size),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION_OF(aligned_alloc),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(_Znwm,
                                              sourcemeta_core_allocator_new),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(_Znam,
                                              sourcemeta_core_allocator_new),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(
            _ZnwmRKSt9nothrow_t, sourcemeta_core_allocator_new_nothrow),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(
            _ZnamRKSt9nothrow_t, sourcemeta_core_allocator_new_nothrow),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(_ZdlPv,
                                              sourcemeta_core_allocator_delete),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(_ZdaPv,
                                              sourcemeta_core_allocator_delete),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(
            _ZdlPvm, sourcemeta_core_allocator_delete_size),
        SOURCEMETA_CORE_ALLOCATOR_REDIRECTION(
            _ZdaPvm, sourcemeta_core_allocator_delete_size)};

#endif
