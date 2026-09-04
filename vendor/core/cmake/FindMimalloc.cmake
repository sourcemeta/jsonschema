if(NOT Mimalloc_FOUND)
  # This module runs before the top-level project sets up its install
  # locations, so the interface that installed consumers see would otherwise
  # be built out of empty directory variables
  include(GNUInstallDirs)

  set(MIMALLOC_DIR "${PROJECT_SOURCE_DIR}/vendor/mimalloc")
  set(MIMALLOC_SOURCE_DIR "${MIMALLOC_DIR}/src")
  set(MIMALLOC_PUBLIC_HEADERS
    "${MIMALLOC_DIR}/include/mimalloc.h"
    "${MIMALLOC_DIR}/include/mimalloc-new-delete.h"
    "${MIMALLOC_DIR}/include/mimalloc-override.h"
    "${MIMALLOC_DIR}/include/mimalloc-stats.h")

  set(MIMALLOC_SOURCES
    "${MIMALLOC_SOURCE_DIR}/alloc.c"
    "${MIMALLOC_SOURCE_DIR}/alloc-aligned.c"
    "${MIMALLOC_SOURCE_DIR}/alloc-posix.c"
    "${MIMALLOC_SOURCE_DIR}/arena.c"
    "${MIMALLOC_SOURCE_DIR}/arena-meta.c"
    "${MIMALLOC_SOURCE_DIR}/bitmap.c"
    "${MIMALLOC_SOURCE_DIR}/heap.c"
    "${MIMALLOC_SOURCE_DIR}/init.c"
    "${MIMALLOC_SOURCE_DIR}/libc.c"
    "${MIMALLOC_SOURCE_DIR}/options.c"
    "${MIMALLOC_SOURCE_DIR}/os.c"
    "${MIMALLOC_SOURCE_DIR}/page.c"
    "${MIMALLOC_SOURCE_DIR}/page-map.c"
    "${MIMALLOC_SOURCE_DIR}/prim/prim.c"
    "${MIMALLOC_SOURCE_DIR}/random.c"
    "${MIMALLOC_SOURCE_DIR}/stats.c"
    "${MIMALLOC_SOURCE_DIR}/theap.c"
    "${MIMALLOC_SOURCE_DIR}/threadlocal.c")

  if(SOURCEMETA_OS_MACOS)
    list(APPEND MIMALLOC_SOURCES
      "${MIMALLOC_SOURCE_DIR}/prim/osx/alloc-override-zone.c")
  endif()

  add_library(mimalloc ${MIMALLOC_SOURCES})
  sourcemeta_add_default_options(PRIVATE mimalloc)

  # Link the resolved thread library rather than the imported target, as the
  # latter obliges every consumer of the exported package to run FindThreads,
  # whose try_compile cannot run inside build systems that read the export by
  # tracing CMake instead of calling it. The imported target also carries a
  # compile option on the platforms whose threads need one, which we set here
  find_package(Threads REQUIRED)
  if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(mimalloc PRIVATE -pthread)
  endif()
  if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(mimalloc PRIVATE "${CMAKE_THREAD_LIBS_INIT}")
  endif()

  target_include_directories(mimalloc PUBLIC
    "$<BUILD_INTERFACE:${MIMALLOC_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  # Define the standard allocation entry points, so that every allocation
  # in the program goes through this library rather than through the
  # allocator that the platform happens to ship with
  target_compile_definitions(mimalloc PRIVATE MI_MALLOC_OVERRIDE)

  # This project compiles its optimised debuggable configuration without
  # disabling assertions, which this library would otherwise read as a
  # request for its own assertions and statistics collection
  target_compile_definitions(mimalloc PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:MI_BUILD_RELEASE>)

  if(SOURCEMETA_OS_MACOS)
    # The only two ways into this platform's allocation path, one of which
    # is only available to a shared library
    target_compile_definitions(mimalloc PRIVATE MI_OSX_ZONE=1 MI_OSX_INTERPOSE=1)
    # Taking over the allocation path here happens after the system has
    # already handed out memory of its own, so deallocation has to determine
    # who owns the pointer rather than assume it is ours
    target_compile_definitions(mimalloc PRIVATE MI_FREE_IS_CHECKED=1)
  endif()

  if(BUILD_SHARED_LIBS)
    target_compile_definitions(mimalloc PRIVATE MI_SHARED_LIB MI_SHARED_LIB_EXPORT)
    # When overriding from a shared library, a pointer handed back to us may
    # have been allocated before this library was loaded, so deallocation
    # has to determine who owns the pointer first
    target_compile_definitions(mimalloc PRIVATE MI_FREE_IS_CHECKED=1)
  else()
    target_compile_definitions(mimalloc PRIVATE MI_STATIC_LIB)
  endif()

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    # Allocation sits on the hot path of every thread, so pay for the
    # cheapest thread-local access sequence that a non-dlopen'ed library
    # is allowed to use
    target_compile_options(mimalloc PRIVATE -ftls-model=initial-exec)
    # This library is the allocator, so the compiler cannot be allowed to
    # reason about the standard entry points or to synthesise calls to them
    target_compile_options(mimalloc PRIVATE -fno-builtin-malloc)
    target_compile_options(mimalloc PRIVATE
      -Wno-conversion -Wno-sign-conversion -Wno-pedantic)
    # The thread local slot table is a trailing single-element array that is
    # over-allocated and indexed past its first element, so the strictest
    # interpretation of what counts as a trailing flexible array would treat
    # every one of those accesses as running off the end of the object
    target_compile_options(mimalloc PRIVATE -fstrict-flex-arrays=0)
  endif()

  if(SOURCEMETA_COMPILER_GCC)
    # Reading an atomic word out of a block whose size the compiler cannot
    # see is reported as if it overflowed the block
    target_compile_options(mimalloc PRIVATE -Wno-stringop-overflow)
  endif()

  if(SOURCEMETA_COMPILER_LLVM)
    target_compile_options(mimalloc PRIVATE -Wno-comma)
  endif()

  set_target_properties(mimalloc
    PROPERTIES
      OUTPUT_NAME mimalloc
      PUBLIC_HEADER "${MIMALLOC_PUBLIC_HEADERS}"
      EXPORT_NAME mimalloc)

  # Nothing refers to the entry points that replace the standard allocator by
  # name, so a linker that only pulls in the archive members it needs would
  # leave the program running on the allocator it was trying to replace
  add_library(mimalloc_interface INTERFACE)
  if(BUILD_SHARED_LIBS)
    target_link_libraries(mimalloc_interface INTERFACE mimalloc)
  else()
    target_link_libraries(mimalloc_interface INTERFACE
      "$<LINK_LIBRARY:WHOLE_ARCHIVE,$<TARGET_NAME:mimalloc>>")
  endif()

  set_target_properties(mimalloc_interface
    PROPERTIES EXPORT_NAME Mimalloc)

  add_library(Mimalloc::Mimalloc ALIAS mimalloc_interface)

  if(SOURCEMETA_CORE_INSTALL)
    install(TARGETS mimalloc mimalloc_interface
      EXPORT mimalloc
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT mimalloc
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mimalloc"
      NAMESPACE Mimalloc::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/mimalloc-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/mimalloc.cmake\")\n"
      "check_required_components(\"mimalloc\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/mimalloc-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/mimalloc"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(Mimalloc_FOUND ON)
endif()
