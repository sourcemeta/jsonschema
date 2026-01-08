sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME uritemplate
  PRIVATE_HEADERS error.h token.h router.h
  SOURCES helpers.h uritemplate.cc uritemplate_router.cc uritemplate_router_view.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME uritemplate)
endif()

target_link_libraries(sourcemeta_core_uritemplate PUBLIC sourcemeta::core::io)
