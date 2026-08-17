# An interpreter for the CLI test DSL, shipped as a program because the suites
# that consume it register each script as a command. It is deliberately not
# installed, as that would make this project a vendor of tools
sourcemeta_executable(NAMESPACE sourcemeta PROJECT core NAME clitest
  SOURCES main.cc clitest.h help.h)

target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::io)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::options)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::process)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::crypto)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::gzip)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::regex)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::unicode)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::diff)
target_link_libraries(sourcemeta_core_clitest
  PRIVATE sourcemeta::core::numeric)
