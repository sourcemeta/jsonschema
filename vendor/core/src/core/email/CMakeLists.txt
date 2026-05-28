sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME email
  SOURCES email.cc helpers.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME email)
endif()

target_link_libraries(sourcemeta_core_email
  PRIVATE sourcemeta::core::dns)
target_link_libraries(sourcemeta_core_email
  PRIVATE sourcemeta::core::ip)
target_link_libraries(sourcemeta_core_email
  PRIVATE sourcemeta::core::unicode)
