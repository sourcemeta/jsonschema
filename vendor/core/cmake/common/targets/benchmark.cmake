function(sourcemeta_benchmark)
  cmake_parse_arguments(SOURCEMETA_BENCHMARK ""
    "NAMESPACE;PROJECT" "SOURCES" ${ARGN})

  sourcemeta_executable(
    NAMESPACE "${SOURCEMETA_BENCHMARK_NAMESPACE}"
    PROJECT "${SOURCEMETA_BENCHMARK_PROJECT}"
    NAME benchmarks
    SOURCES "${SOURCEMETA_BENCHMARK_SOURCES}"
    OUTPUT TARGET_NAME)

  target_link_libraries("${TARGET_NAME}" PRIVATE sourcemeta::core::benchmark)
  target_link_libraries("${TARGET_NAME}" PRIVATE sourcemeta::core::benchmark_main)
endfunction()
