sourcemeta_library(NAMESPACE sourcemeta PROJECT jsonbinpack NAME numeric
  FOLDER "JSON BinPack/Numeric"
  PRIVATE_HEADERS integral.h real.h zigzag.h)

if(JSONBINPACK_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT jsonbinpack NAME numeric)
endif()
