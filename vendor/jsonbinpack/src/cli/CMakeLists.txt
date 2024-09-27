configure_file(version.h.in version.h @ONLY)
add_executable(sourcemeta_jsonbinpack_cli main.cc commands.h
  command_help.cc command_version.cc
  command_canonicalize.cc command_compile.cc
  command_encode.cc command_decode.cc
  defaults.h version.h.in)

noa_add_default_options(PRIVATE sourcemeta_jsonbinpack_cli)

target_link_libraries(sourcemeta_jsonbinpack_cli PRIVATE sourcemeta::jsonbinpack::compiler)
target_link_libraries(sourcemeta_jsonbinpack_cli PRIVATE sourcemeta::jsonbinpack::runtime)
target_link_libraries(sourcemeta_jsonbinpack_cli PRIVATE sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_jsonbinpack_cli PRIVATE sourcemeta::jsontoolkit::jsonschema)

# To find the generated version file
target_include_directories(sourcemeta_jsonbinpack_cli PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
# Set a friendly name for the binary
set_target_properties(sourcemeta_jsonbinpack_cli PROPERTIES
  OUTPUT_NAME jsonbinpack
  FOLDER "JSON BinPack/CLI")

install(TARGETS sourcemeta_jsonbinpack_cli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  COMPONENT sourcemeta_jsonbinpack)
