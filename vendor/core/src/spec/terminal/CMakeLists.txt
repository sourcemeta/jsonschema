sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME terminal
  SOURCES terminal.cc terminal_internal.h terminal_ansi_escapes.h)

if(WIN32)
  target_sources(sourcemeta_core_terminal PRIVATE terminal_windows.cc)
else()
  target_sources(sourcemeta_core_terminal PRIVATE terminal_posix.cc)
endif()

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME terminal)
endif()
