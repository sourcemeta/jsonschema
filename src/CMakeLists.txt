add_executable(jsonschema_cli
  main.cc configure.h.in command.h
  utils.h utils.cc
  command_fmt.cc
  command_frame.cc
  command_bundle.cc
  command_test.cc
  command_lint.cc
  command_metaschema.cc
  command_validate.cc
  command_identify.cc
  command_canonicalize.cc
  command_encode.cc
  command_decode.cc)

noa_add_default_options(PRIVATE jsonschema_cli)
set_target_properties(jsonschema_cli PROPERTIES OUTPUT_NAME jsonschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::uri)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::json)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::jsonl)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsontoolkit::jsonschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::alterschema::engine)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::alterschema::linter)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::hydra::httpclient)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsonbinpack::compiler)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsonbinpack::runtime)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::compiler)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::evaluator)

configure_file(configure.h.in configure.h @ONLY)
target_include_directories(jsonschema_cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

include(GNUInstallDirs)
install(TARGETS jsonschema_cli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT sourcemeta_jsonschema)
