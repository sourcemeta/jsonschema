noa_library(NAMESPACE sourcemeta PROJECT jsonbinpack NAME runtime
  FOLDER "JSON BinPack/Runtime"
  PRIVATE_HEADERS
    decoder.h decoder_basic.h
    encoder.h encoder_basic.h encoder_context.h
    plan.h plan_wrap.h parser.h
    varint.h
  SOURCES runtime_parser.cc runtime_parser_v1.h)

if(JSONBINPACK_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT jsonbinpack NAME runtime)
endif()

target_link_libraries(sourcemeta_jsonbinpack_runtime PUBLIC
  sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_jsonbinpack_runtime PUBLIC
  sourcemeta::jsonbinpack::numeric)
