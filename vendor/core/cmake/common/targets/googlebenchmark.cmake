function(sourcemeta_googlebenchmark)
  cmake_parse_arguments(SOURCEMETA_GOOGLEBENCHMARK ""
    "NAMESPACE;PROJECT" "SOURCES" ${ARGN})

  sourcemeta_executable(
    NAMESPACE "${SOURCEMETA_GOOGLEBENCHMARK_NAMESPACE}"
    PROJECT "${SOURCEMETA_GOOGLEBENCHMARK_PROJECT}"
    NAME benchmark
    SOURCES "${SOURCEMETA_GOOGLEBENCHMARK_SOURCES}"
    OUTPUT TARGET_NAME)

  target_link_libraries("${TARGET_NAME}" PRIVATE benchmark::benchmark)
  target_link_libraries("${TARGET_NAME}" PRIVATE benchmark::benchmark_main)
endfunction()
