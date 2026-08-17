# An interpreter for the CLI test DSL, shipped as a program because the suites
# that consume it register each script as a command
sourcemeta_executable(NAMESPACE sourcemeta PROJECT core NAME clitest
  SOURCES main.cc clitest.h help.h)
add_executable(sourcemeta::core::clitest ALIAS sourcemeta_core_clitest)
set_target_properties(sourcemeta_core_clitest
  PROPERTIES EXPORT_NAME core::clitest)

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

if(SOURCEMETA_CORE_INSTALL)
  include(GNUInstallDirs)
  # A program that other programs run rather than one a person types, so it goes
  # where those belong instead of onto the path of everything that installs this
  install(TARGETS sourcemeta_core_clitest
    EXPORT sourcemeta_core_clitest
    RUNTIME DESTINATION "${CMAKE_INSTALL_LIBEXECDIR}/sourcemeta/core"
      COMPONENT sourcemeta_core_dev)
  install(EXPORT sourcemeta_core_clitest
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/core"
    NAMESPACE sourcemeta::
    COMPONENT sourcemeta_core_dev)
endif()
