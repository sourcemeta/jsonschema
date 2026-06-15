sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME http
  PRIVATE_HEADERS problem.h status.h method.h message.h error.h
  SOURCES helpers.h problem.cc match_accept.cc match_accept_language.cc
          negotiate_encoding.cc from_date.cc format_link.cc field_list.cc
          accept_includes_all.cc content_type_matches.cc parse_bearer.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME http)
endif()

target_link_libraries(sourcemeta_core_http PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_http PUBLIC sourcemeta::core::text)
target_link_libraries(sourcemeta_core_http PRIVATE sourcemeta::core::time)
