noa_library(NAMESPACE sourcemeta PROJECT alterschema NAME linter
  FOLDER "AlterSchema/Linter"
  SOURCES linter.cc
    # Modernize
    modernize/enum_to_const.h

    # Antipattern
    antipattern/const_with_type.h
    antipattern/enum_with_type.h

    # Simplify
    simplify/single_type_array.h

    # Redundant
    redundant/additional_properties_default.h
    redundant/content_media_type_without_encoding.h
    redundant/content_schema_default.h
    redundant/content_schema_without_media_type.h
    redundant/else_without_if.h
    redundant/items_array_default.h
    redundant/items_schema_default.h
    redundant/max_contains_without_contains.h
    redundant/min_contains_without_contains.h
    redundant/then_without_if.h
    redundant/unevaluated_items_default.h
    redundant/unevaluated_properties_default.h)

if(ALTERSCHEMA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT alterschema NAME linter)
endif()

target_link_libraries(sourcemeta_alterschema_linter PUBLIC
  sourcemeta::alterschema::engine)
