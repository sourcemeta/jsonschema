add_executable(jsonschema_cli
  main.cc configure.h.in command.h
  utils.h utils.cc
  command_help.cc
  command_fmt.cc
  command_frame.cc
  command_lint.cc
  command_validate.cc
  lint/enum_with_type.h)
intelligence_jsonschema_add_compile_options(jsonschema_cli)
set_target_properties(jsonschema_cli PROPERTIES OUTPUT_NAME jsonschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::json)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::jsonschema)

configure_file(configure.h.in configure.h @ONLY)
target_include_directories(jsonschema_cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

include(GNUInstallDirs)
install(TARGETS jsonschema_cli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT intelligence_jsonschema)
