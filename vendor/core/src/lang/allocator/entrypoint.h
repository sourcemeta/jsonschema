#ifndef SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT_H_
#define SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT_H_

// This library hides its symbols by default, which the entry points it defines
// cannot afford, as the whole point of them is that a program already calls
// them by name. Link time optimisation has to keep them for the same reason,
// even though nothing in this library refers to them
#define SOURCEMETA_CORE_ALLOCATOR_ENTRYPOINT                                   \
  __attribute__((visibility("default"), used))

// Defining the platform's own names only replaces them for the program that
// does the defining. On one platform a library that is not the program itself
// cannot manage it, as every other library goes on binding to the names it was
// built against, and those are the bindings that hand us memory to release.
// There the platform is asked to redirect them instead, which it only does for
// a name the redirecting library does not also define, so under that
// arrangement these entry points answer to a name of their own and the
// redirection is what puts them in the platform's place
#ifdef SOURCEMETA_CORE_ALLOCATOR_REDIRECT
#define SOURCEMETA_CORE_ALLOCATOR_REPLACES(name)                               \
  sourcemeta_core_allocator_##name
#else
#define SOURCEMETA_CORE_ALLOCATOR_REPLACES(name) name
#endif

#endif
