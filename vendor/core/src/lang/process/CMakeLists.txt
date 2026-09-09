sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME process
  PRIVATE_HEADERS error.h usage.h
  SOURCES spawn.cc usage.cc command_line.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME process)
endif()

target_link_libraries(sourcemeta_core_process PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_process PRIVATE sourcemeta::core::numeric)

if(WIN32)
  target_link_libraries(sourcemeta_core_process PRIVATE psapi)
endif()
