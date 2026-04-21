sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME codegen
  FOLDER "Blaze/Codegen"
  PRIVATE_HEADERS error.h typescript.h
  SOURCES
    codegen.cc
    codegen_symbol.cc
    codegen_default_compiler.h
    codegen_typescript.cc
    codegen_mangle.cc)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME codegen)
endif()

target_link_libraries(sourcemeta_blaze_codegen PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_codegen PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_codegen PRIVATE
  sourcemeta::blaze::alterschema)
