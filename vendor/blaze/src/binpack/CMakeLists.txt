sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME binpack
  FOLDER "Blaze/BinPack"
  PRIVATE_HEADERS
    error.h
    decoder.h
    encoder.h
    input_stream.h
    output_stream.h
    encoder_cache.h
    encoding.h
  SOURCES
    compile.cc
    mapper/enum_8_bit.h
    mapper/enum_8_bit_top_level.h
    mapper/enum_arbitrary.h
    mapper/enum_singleton.h
    mapper/integer_bounded_8_bit.h
    mapper/integer_bounded_greater_than_8_bit.h
    mapper/integer_bounded_multiplier_8_bit.h
    mapper/integer_bounded_multiplier_greater_than_8_bit.h
    mapper/integer_lower_bound.h
    mapper/integer_lower_bound_multiplier.h
    mapper/integer_unbound.h
    mapper/integer_unbound_multiplier.h
    mapper/integer_upper_bound.h
    mapper/integer_upper_bound_multiplier.h
    mapper/number_arbitrary.h

    input_stream.cc
    output_stream.cc
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

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME binpack)
endif()

target_link_libraries(sourcemeta_blaze_binpack PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_binpack PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_binpack PUBLIC
  sourcemeta::core::numeric)
target_link_libraries(sourcemeta_blaze_binpack PUBLIC
  sourcemeta::core::io)
target_link_libraries(sourcemeta_blaze_binpack PRIVATE
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_binpack PRIVATE
  sourcemeta::blaze::alterschema)
target_link_libraries(sourcemeta_blaze_binpack PRIVATE
  sourcemeta::blaze::canonicalizer)
