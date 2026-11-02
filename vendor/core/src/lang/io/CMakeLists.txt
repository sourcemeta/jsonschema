sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME io
  PRIVATE_HEADERS atomic.h binary.h bytestream.h error.h fileview.h temporary.h
  SOURCES io.cc io_atomic.cc io_binary.cc io_bytestream.cc io_fileview.cc io_temporary.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME io)
endif()

target_link_libraries(sourcemeta_core_io
  PUBLIC sourcemeta::core::error)
target_link_libraries(sourcemeta_core_io
  PUBLIC sourcemeta::core::numeric)
