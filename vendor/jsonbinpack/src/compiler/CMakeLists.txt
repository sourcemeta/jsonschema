sourcemeta_library(NAMESPACE sourcemeta PROJECT jsonbinpack NAME compiler
  FOLDER "JSON BinPack/Compiler"
  SOURCES
    encoding.h compiler.cc
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
    mapper/number_arbitrary.h)

if(JSONBINPACK_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT jsonbinpack NAME compiler)
endif()

target_link_libraries(sourcemeta_jsonbinpack_compiler PRIVATE
  sourcemeta::jsonbinpack::numeric)
target_link_libraries(sourcemeta_jsonbinpack_compiler PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_jsonbinpack_compiler PRIVATE
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_jsonbinpack_compiler PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_jsonbinpack_compiler PRIVATE
  sourcemeta::core::alterschema)
