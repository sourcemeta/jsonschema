noa_library(NAMESPACE sourcemeta PROJECT noa NAME regex
  FOLDER "Noa/Regex")

if(NOA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT noa NAME regex)
endif()

target_link_libraries(sourcemeta_noa_regex INTERFACE Boost::regex)
