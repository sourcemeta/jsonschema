find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(SOURCEMETA_CORE_UNICODE_UCD_DIR
  "${core_SOURCE_DIR}/vendor/unicodetools/unicodetools/data/ucd/dev")

set(SOURCEMETA_CORE_UNICODE_DATA_HEADER
  "${CMAKE_CURRENT_BINARY_DIR}/unicode_data.h")

add_custom_command(
  OUTPUT "${SOURCEMETA_CORE_UNICODE_DATA_HEADER}"
  COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py"
    "${SOURCEMETA_CORE_UNICODE_DATA_HEADER}"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/PropertyValueAliases.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedCombiningClass.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedJoiningType.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedBidiClass.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/Scripts.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedGeneralCategory.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/DerivedNormalizationProps.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/UnicodeData.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/CompositionExclusions.txt"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/codegen.py"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/PropertyValueAliases.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedCombiningClass.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedJoiningType.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedBidiClass.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/Scripts.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/extracted/DerivedGeneralCategory.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/DerivedNormalizationProps.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/UnicodeData.txt"
    "${SOURCEMETA_CORE_UNICODE_UCD_DIR}/CompositionExclusions.txt"
  COMMENT "Generating Unicode property tables"
  VERBATIM)

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME unicode
  PRIVATE_HEADERS ucd.h
  SOURCES unicode.cc nfc.cc "${SOURCEMETA_CORE_UNICODE_DATA_HEADER}")

target_include_directories(sourcemeta_core_unicode PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME unicode)
endif()
