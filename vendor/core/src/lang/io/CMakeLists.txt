sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME io
  PRIVATE_HEADERS error.h fileview.h temporary.h
  SOURCES io.cc io_fileview.cc io_temporary.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME io)
endif()
