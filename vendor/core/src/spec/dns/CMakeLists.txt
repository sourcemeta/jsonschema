sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME dns
  SOURCES hostname.cc idn_hostname.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME dns)
endif()

target_link_libraries(sourcemeta_core_dns
  PRIVATE sourcemeta::core::unicode
  PRIVATE sourcemeta::core::punycode
  PRIVATE sourcemeta::core::idna
  PRIVATE sourcemeta::core::text)
