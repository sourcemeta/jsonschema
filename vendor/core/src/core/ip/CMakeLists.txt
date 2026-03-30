sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME ip
  SOURCES ipv4.cc ipv6.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME ip)
endif()
