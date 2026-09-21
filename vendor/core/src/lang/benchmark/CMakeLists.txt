sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME benchmark
  SOURCES benchmark.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME benchmark)
endif()

target_link_libraries(sourcemeta_core_benchmark
  PUBLIC sourcemeta::core::preprocessor)
target_link_libraries(sourcemeta_core_benchmark
  PRIVATE sourcemeta::core::io)
target_link_libraries(sourcemeta_core_benchmark
  PRIVATE sourcemeta::core::json)
target_link_libraries(sourcemeta_core_benchmark
  PRIVATE sourcemeta::core::options)
target_link_libraries(sourcemeta_core_benchmark
  PRIVATE sourcemeta::core::process)

# A dedicated, always-static entry point library. Linking it into every
# benchmark executable provides a default `main()` through static archive
# resolution, while a suite that defines its own `main()` transparently
# overrides it. This mirrors how the test framework ships its own entry point.
add_library(sourcemeta_core_benchmark_main STATIC benchmark_main.cc)
add_library(sourcemeta::core::benchmark_main ALIAS sourcemeta_core_benchmark_main)
sourcemeta_add_default_options(PRIVATE sourcemeta_core_benchmark_main)
# Public so the dependency propagates to consumers and CMake orders this archive
# ahead of the framework, keeping the entry point's reference to the runner
# resolvable at link time
target_link_libraries(sourcemeta_core_benchmark_main
  PUBLIC sourcemeta::core::benchmark)
set_target_properties(sourcemeta_core_benchmark_main
  PROPERTIES EXPORT_NAME core::benchmark_main)

# We don't want consumers to be bothered with this
if(PROJECT_IS_TOP_LEVEL)
  sourcemeta_clang_tidy_attempt_enable(TARGET sourcemeta_core_benchmark_main)
endif()

if(SOURCEMETA_CORE_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS sourcemeta_core_benchmark_main
    EXPORT sourcemeta_core_benchmark_main
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT sourcemeta_core_dev)
  install(EXPORT sourcemeta_core_benchmark_main
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/core"
    NAMESPACE sourcemeta::
    COMPONENT sourcemeta_core_dev)
endif()
