sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME regex
  SOURCES regex.cc preprocess.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME regex)
endif()

target_link_libraries(sourcemeta_core_regex PRIVATE PCRE2::pcre2)
