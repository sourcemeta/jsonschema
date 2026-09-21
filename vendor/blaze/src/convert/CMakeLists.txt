sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME convert
  FOLDER "Blaze/Convert"
  PRIVATE_HEADERS error.h
  SOURCES convert.cc
    rule.h
    helpers.h

    # Rules
    rules/definitions_to_defs.h
    rules/draft_official_dialect_with_https.h
    rules/draft_official_dialect_without_empty_fragment.h
    rules/empty_object_as_true.h
    rules/enum_to_const.h
    rules/metaschema_vocabulary.h
    rules/prefix_promoted_2020_12_keywords.h
    rules/prefix_promoted_draft_2019_09_keywords.h
    rules/prefix_promoted_draft_4_keywords.h
    rules/prefix_promoted_draft_6_keywords.h
    rules/prefix_promoted_draft_7_keywords.h
    rules/upgrade_2019_09_to_2020_12.h
    rules/upgrade_dialect_override_cleanup.h
    rules/upgrade_draft_3_to_draft_4.h
    rules/upgrade_draft_4_to_draft_6.h
    rules/upgrade_draft_6_to_draft_7.h
    rules/upgrade_draft_7_to_draft_2019_09.h)

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME convert)
endif()

target_link_libraries(sourcemeta_blaze_convert PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_blaze_convert PRIVATE
  sourcemeta::core::uri)
