sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME stacktrace
  PRIVATE_HEADERS
  SOURCES stacktrace.cc stacktrace_posix.h stacktrace_windows.h)

# Counters are written out when a program exits, which a program dying on a
# fatal signal never does, so the crash handler below reads as unexercised even
# though the end to end tests of this module drive it on a real signal. Keeping
# the counters up to date as they change instead costs the compiler cache for
# the translation units carrying it, so it is asked for here rather than across
# the project, and only where the build is already instrumented
if(UNIX AND CMAKE_CXX_FLAGS MATCHES "-fprofile-instr-generate")
  target_compile_options(sourcemeta_core_stacktrace
    PRIVATE "SHELL:-mllvm -runtime-counter-relocation")
endif()

if(UNIX)
  target_link_libraries(sourcemeta_core_stacktrace PRIVATE ${CMAKE_DL_LIBS})
  target_link_options(sourcemeta_core_stacktrace INTERFACE
    "$<$<PLATFORM_ID:Linux>:-rdynamic>"
    "$<$<PLATFORM_ID:Darwin>:-Wl,-export_dynamic>")
elseif(WIN32)
  target_link_libraries(sourcemeta_core_stacktrace PRIVATE dbghelp)
endif()

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME stacktrace)
endif()
