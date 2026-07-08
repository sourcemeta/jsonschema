# Codegen
set(SOURCEMETA_CORE_IDNA_UCD_DIR
  "${core_SOURCE_DIR}/vendor/unicodetools/unicodetools/data/idna/dev")
set(SOURCEMETA_CORE_IDNA_DATA_HEADER
  "${CMAKE_CURRENT_BINARY_DIR}/idna_data.h")
sourcemeta_executable(NAMESPACE sourcemeta PROJECT core NAME idna_codegen
  OUTPUT SOURCEMETA_CORE_IDNA_CODEGEN_TARGET
  SOURCES codegen.cc include/sourcemeta/core/idna_ucd.h)
target_link_libraries("${SOURCEMETA_CORE_IDNA_CODEGEN_TARGET}" PRIVATE
  sourcemeta::core::io
  sourcemeta::core::options
  sourcemeta::core::numeric
  sourcemeta::core::text)
target_include_directories("${SOURCEMETA_CORE_IDNA_CODEGEN_TARGET}" PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_custom_command(
  OUTPUT "${SOURCEMETA_CORE_IDNA_DATA_HEADER}"
  COMMAND "${SOURCEMETA_CORE_IDNA_CODEGEN_TARGET}"
    "${SOURCEMETA_CORE_IDNA_DATA_HEADER}"
    "${SOURCEMETA_CORE_IDNA_UCD_DIR}/Idna2008.txt"
    "${SOURCEMETA_CORE_IDNA_UCD_DIR}/IdnaMappingTable.txt"
  DEPENDS
    "${SOURCEMETA_CORE_IDNA_CODEGEN_TARGET}"
    "${SOURCEMETA_CORE_IDNA_UCD_DIR}/Idna2008.txt"
    "${SOURCEMETA_CORE_IDNA_UCD_DIR}/IdnaMappingTable.txt"
  COMMENT "Generating IDNA property and mapping tables"
  VERBATIM)

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME idna
  PRIVATE_HEADERS ucd.h
  SOURCES idna.cc "${SOURCEMETA_CORE_IDNA_DATA_HEADER}")

target_include_directories(sourcemeta_core_idna PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME idna)
endif()

target_link_libraries(sourcemeta_core_idna PRIVATE sourcemeta::core::unicode)
target_link_libraries(sourcemeta_core_idna PRIVATE sourcemeta::core::punycode)
