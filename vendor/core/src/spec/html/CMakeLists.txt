# Codegen
set(SOURCEMETA_CORE_HTML_ENTITIES_PATH
  "${core_SOURCE_DIR}/vendor/whatwg-html-build/entities/out/entities.json")
set(SOURCEMETA_CORE_HTML_ENTITIES_HEADER
  "${CMAKE_CURRENT_BINARY_DIR}/html_entities.h")
sourcemeta_executable(NAMESPACE sourcemeta PROJECT core NAME html_codegen
  OUTPUT SOURCEMETA_CORE_HTML_CODEGEN_TARGET
  SOURCES codegen.cc entity_hash.h)
target_link_libraries("${SOURCEMETA_CORE_HTML_CODEGEN_TARGET}" PRIVATE
  sourcemeta::core::io
  sourcemeta::core::json
  sourcemeta::core::options
  sourcemeta::core::unicode)
add_custom_command(
  OUTPUT "${SOURCEMETA_CORE_HTML_ENTITIES_HEADER}"
  COMMAND "${SOURCEMETA_CORE_HTML_CODEGEN_TARGET}"
    "${SOURCEMETA_CORE_HTML_ENTITIES_HEADER}"
    "${SOURCEMETA_CORE_HTML_ENTITIES_PATH}"
  DEPENDS
    "${SOURCEMETA_CORE_HTML_CODEGEN_TARGET}"
    "${SOURCEMETA_CORE_HTML_ENTITIES_PATH}"
  COMMENT "Generating HTML named character references"
  VERBATIM)

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME html
  PRIVATE_HEADERS buffer.h entity.h escape.h writer.h
  SOURCES entity.cc entity_hash.h escape.cc writer.cc
    "${SOURCEMETA_CORE_HTML_ENTITIES_HEADER}")

target_include_directories(sourcemeta_core_html PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME html)
endif()

target_link_libraries(sourcemeta_core_html PUBLIC sourcemeta::core::preprocessor)
