sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME stacktrace
  PRIVATE_HEADERS
  SOURCES stacktrace.cc stacktrace_posix.h stacktrace_windows.h)

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
