sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME ip
  SOURCES ip.cc ipv4.cc ipv6.cc ip_helpers.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME ip)
endif()

target_link_libraries(sourcemeta_core_ip PRIVATE sourcemeta::core::text)
