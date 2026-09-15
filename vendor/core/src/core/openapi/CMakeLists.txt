sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME openapi
  PRIVATE_HEADERS error.h
  SOURCES helpers.h reference.h example.h content.h link.h response.h
          parameter.h request_body.h security.h path_item.h paths.h
          components.h external_documentation.h info.h server.h tag.h
          document.h frame.cc version.cc)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME openapi)
endif()

target_link_libraries(sourcemeta_core_openapi PUBLIC sourcemeta::core::json)
target_link_libraries(sourcemeta_core_openapi PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_core_openapi PUBLIC
  sourcemeta::core::jsonschema)
target_link_libraries(sourcemeta_core_openapi PRIVATE sourcemeta::core::uri)
target_link_libraries(sourcemeta_core_openapi PRIVATE sourcemeta::core::email)
target_link_libraries(sourcemeta_core_openapi PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_openapi PRIVATE sourcemeta::core::http)
target_link_libraries(sourcemeta_core_openapi PRIVATE
  sourcemeta::core::unicode)
target_link_libraries(sourcemeta_core_openapi PRIVATE
  sourcemeta::core::uritemplate)
