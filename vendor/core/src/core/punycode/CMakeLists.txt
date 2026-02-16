sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME punycode
  PRIVATE_HEADERS error.h
  SOURCES punycode.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME punycode)
endif()

target_link_libraries(sourcemeta_core_punycode PRIVATE sourcemeta::core::unicode)
