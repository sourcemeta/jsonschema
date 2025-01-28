sourcemeta_library(NAMESPACE sourcemeta PROJECT jsonbinpack NAME runtime
  FOLDER "JSON BinPack/Runtime"
  PRIVATE_HEADERS
    decoder.h
    encoder.h
    input_stream.h
    output_stream.h
    encoder_cache.h
    encoding.h
  SOURCES
    input_stream.cc
    output_stream.cc
    varint.h
    unreachable.h
    cache.cc

    loader.cc
    loader_v1_any.h
    loader_v1_array.h
    loader_v1_integer.h
    loader_v1_number.h
    loader_v1_string.h

    decoder_any.cc
    decoder_array.cc
    decoder_common.cc
    decoder_integer.cc
    decoder_number.cc
    decoder_object.cc
    decoder_string.cc
    encoder_any.cc
    encoder_array.cc
    encoder_common.cc
    encoder_integer.cc
    encoder_number.cc
    encoder_object.cc
    encoder_string.cc)

if(JSONBINPACK_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT jsonbinpack NAME runtime)
endif()

target_link_libraries(sourcemeta_jsonbinpack_runtime PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_jsonbinpack_runtime PUBLIC
  sourcemeta::jsonbinpack::numeric)
