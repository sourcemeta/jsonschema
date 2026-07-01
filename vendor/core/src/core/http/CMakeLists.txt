if(SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL)
  set(SOURCEMETA_CORE_HTTP_CLIENT_SOURCE client_curl.cc)
elseif(APPLE)
  set(SOURCEMETA_CORE_HTTP_CLIENT_SOURCE client_darwin.mm)
elseif(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "MSYS")
  set(SOURCEMETA_CORE_HTTP_CLIENT_SOURCE client_windows.cc)
else()
  set(SOURCEMETA_CORE_HTTP_CLIENT_SOURCE client_curl.cc)
endif()

sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME http
  PRIVATE_HEADERS problem.h status.h method.h message.h error.h system.h
  SOURCES helpers.h problem.cc match_accept.cc match_accept_language.cc
          negotiate_encoding.cc from_date.cc format_link.cc field_list.cc
          accept_includes_all.cc content_type_matches.cc parse_bearer.cc
          cache_control_max_age.cc
          ${SOURCEMETA_CORE_HTTP_CLIENT_SOURCE})

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME http)
endif()

target_link_libraries(sourcemeta_core_http PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_http PUBLIC sourcemeta::core::text)
target_link_libraries(sourcemeta_core_http PRIVATE sourcemeta::core::time)

if(SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL)
  find_package(CURL REQUIRED)
  target_compile_definitions(sourcemeta_core_http
    PRIVATE SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL)
  target_link_libraries(sourcemeta_core_http PRIVATE CURL::libcurl)
elseif(APPLE)
  target_link_libraries(sourcemeta_core_http PRIVATE "-framework Foundation")
elseif(WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "MSYS")
  target_link_libraries(sourcemeta_core_http PRIVATE winhttp)
  target_link_libraries(sourcemeta_core_http PRIVATE sourcemeta::core::unicode)
else()
  target_link_libraries(sourcemeta_core_http PRIVATE ${CMAKE_DL_LIBS})
endif()
