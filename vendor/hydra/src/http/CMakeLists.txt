noa_library(NAMESPACE sourcemeta PROJECT hydra NAME http
  FOLDER "Hydra/HTTP"
  PRIVATE_HEADERS method.h status.h error.h header.h time.h
  SOURCES method.cc status.cc error.cc header.cc time.cc)

if(HYDRA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT hydra NAME http)
endif()

sourcemeta_hydra_add_compile_options(sourcemeta_hydra_http)
