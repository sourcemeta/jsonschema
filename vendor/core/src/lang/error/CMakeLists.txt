sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME error
  PRIVATE_HEADERS base.h file.h
  SOURCES error.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME error)
endif()

target_link_libraries(sourcemeta_core_error
  PUBLIC sourcemeta::core::text)
