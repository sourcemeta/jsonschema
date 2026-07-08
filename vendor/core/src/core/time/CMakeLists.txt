sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME time
  SOURCES helpers.h imf_fixdate.cc rfc850_date.cc asctime.cc
          iso8601_basic.cc
          unix_timestamp.cc rfc3339_datetime.cc rfc3339_fulldate.cc
          rfc3339_fulltime.cc rfc3339_partialtime_no_secfrac.cc
          rfc3339_duration.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME time)
endif()

target_link_libraries(sourcemeta_core_time PRIVATE sourcemeta::core::numeric)
