sourcemeta_library(NAMESPACE sourcemeta PROJECT codegen NAME ir
  FOLDER "Codegen/IR"
  PRIVATE_HEADERS error.h
  SOURCES ir.cc ir_symbol.cc ir_default_compiler.h)

if(CODEGEN_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT codegen NAME ir)
endif()

target_link_libraries(sourcemeta_codegen_ir PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_codegen_ir PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_codegen_ir PRIVATE
  sourcemeta::core::alterschema)
