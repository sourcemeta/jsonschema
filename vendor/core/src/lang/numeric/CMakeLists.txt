sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME numeric
  PRIVATE_HEADERS error.h decimal.h parse.h SOURCES decimal.cc parse.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME numeric)
endif()

target_link_libraries(sourcemeta_core_numeric PRIVATE mpdecimal)
