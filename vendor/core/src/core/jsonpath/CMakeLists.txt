sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME jsonpath
  PRIVATE_HEADERS error.h
  SOURCES jsonpath.cc parser.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME jsonpath)
endif()

target_link_libraries(sourcemeta_core_jsonpath PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_core_jsonpath PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_jsonpath PUBLIC
  sourcemeta::core::regex)
target_link_libraries(sourcemeta_core_jsonpath PRIVATE
  sourcemeta::core::numeric)
target_link_libraries(sourcemeta_core_jsonpath PRIVATE
  sourcemeta::core::text)
target_link_libraries(sourcemeta_core_jsonpath PRIVATE
  sourcemeta::core::unicode)
