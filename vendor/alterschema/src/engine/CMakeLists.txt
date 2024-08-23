noa_library(NAMESPACE sourcemeta PROJECT alterschema NAME engine
  FOLDER "AlterSchema/Engine"
  PRIVATE_HEADERS bundle.h rule.h transformer.h
  SOURCES bundle.cc rule.cc transformer.cc)

if(ALTERSCHEMA_INSTALL)
  noa_library_install(NAMESPACE sourcemeta PROJECT alterschema NAME engine)
endif()

target_link_libraries(sourcemeta_alterschema_engine PUBLIC
  sourcemeta::jsontoolkit::json)
target_link_libraries(sourcemeta_alterschema_engine PUBLIC
  sourcemeta::jsontoolkit::jsonpointer)
target_link_libraries(sourcemeta_alterschema_engine PUBLIC
  sourcemeta::jsontoolkit::jsonschema)
