sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME test
  SOURCES test.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME test)
endif()

target_link_libraries(sourcemeta_core_test
  PUBLIC sourcemeta::core::numeric)
target_link_libraries(sourcemeta_core_test
  PRIVATE sourcemeta::core::options)
target_link_libraries(sourcemeta_core_test
  PRIVATE sourcemeta::core::stacktrace)

# A dedicated, always-static entry point library. Linking it into every test
# executable provides a default `main()` through static archive resolution,
# while a suite that defines its own `main()` transparently overrides it. This
# mirrors how GoogleTest ships `gtest_main`.
add_library(sourcemeta_core_test_main STATIC test_main.cc)
add_library(sourcemeta::core::test_main ALIAS sourcemeta_core_test_main)
sourcemeta_add_default_options(PRIVATE sourcemeta_core_test_main)
# Public so the dependency propagates to consumers and CMake orders this archive
# ahead of the framework, keeping the entry point's reference to the runner
# resolvable at link time
target_link_libraries(sourcemeta_core_test_main
  PUBLIC sourcemeta::core::test)
set_target_properties(sourcemeta_core_test_main
  PROPERTIES EXPORT_NAME core::test_main)

if(SOURCEMETA_CORE_INSTALL)
  include(GNUInstallDirs)
  install(TARGETS sourcemeta_core_test_main
    EXPORT sourcemeta_core_test_main
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    COMPONENT sourcemeta_core_dev)
  install(EXPORT sourcemeta_core_test_main
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/core"
    NAMESPACE sourcemeta::
    COMPONENT sourcemeta_core_dev)
endif()
