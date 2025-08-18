sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME build
  PRIVATE_HEADERS types.h adapter_filesystem.h SOURCES adapter_filesystem.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME build)
endif()

target_link_libraries(sourcemeta_core_build PRIVATE sourcemeta::core::io)
