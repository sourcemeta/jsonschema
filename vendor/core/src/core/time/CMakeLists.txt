sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME time
  SOURCES gmt.cc rfc3339_datetime.cc rfc3339_fulldate.cc rfc3339_fulltime.cc
          rfc3339_duration.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME time)
endif()

target_link_libraries(sourcemeta_core_time PRIVATE sourcemeta::core::numeric)
