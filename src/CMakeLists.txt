add_executable(jsonschema_cli
  main.cc configure.h.in command.h
  utils.h error.h logger.h configuration.h input.h resolver.h
  command_fmt.cc
  command_inspect.cc
  command_bundle.cc
  command_test.cc
  command_lint.cc
  command_metaschema.cc
  command_validate.cc
  command_encode.cc
  command_decode.cc
  command_compile.cc)

sourcemeta_add_default_options(PRIVATE jsonschema_cli)
set_target_properties(jsonschema_cli PROPERTIES OUTPUT_NAME jsonschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::io)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::uri)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::json)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::jsonl)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::jsonpointer)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::jsonschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::yaml)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::alterschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::editorschema)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::options)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::core::schemaconfig)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsonbinpack::compiler)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::jsonbinpack::runtime)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::compiler)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::evaluator)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::linter)
target_link_libraries(jsonschema_cli PRIVATE sourcemeta::blaze::output)
target_link_libraries(jsonschema_cli PRIVATE CPR::cpr)

configure_file(configure.h.in configure.h @ONLY)
target_include_directories(jsonschema_cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

include(GNUInstallDirs)
install(TARGETS jsonschema_cli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT sourcemeta_jsonschema)

install(FILES "${PROJECT_SOURCE_DIR}/completion/jsonschema.bash"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/bash-completion/completions"
  RENAME jsonschema
  COMPONENT sourcemeta_jsonschema)

install(FILES "${PROJECT_SOURCE_DIR}/completion/jsonschema.zsh"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/zsh/site-functions"
  RENAME _jsonschema
  COMPONENT sourcemeta_jsonschema)
