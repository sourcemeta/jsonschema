set(KNOWN_RESOLVER_INPUT "${CMAKE_CURRENT_SOURCE_DIR}/known_resolver.in.cc")
set(KNOWN_RESOLVER_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/known_resolver.cc")
include(./known_resolver.cmake)

sourcemeta_library(NAMESPACE sourcemeta PROJECT blaze NAME foundation
  FOLDER "Blaze/Foundation"
  PRIVATE_HEADERS walker.h error.h
                  types.h vocabularies.h
  SOURCES foundation.cc vocabularies.cc known_walker.cc
          walker.cc helpers.h
    "${CMAKE_CURRENT_BINARY_DIR}/known_resolver.cc")

if(BLAZE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT blaze NAME foundation)
endif()

target_link_libraries(sourcemeta_blaze_foundation PUBLIC
  sourcemeta::core::json)
target_link_libraries(sourcemeta_blaze_foundation PUBLIC
  sourcemeta::core::jsonpointer)
target_link_libraries(sourcemeta_blaze_foundation PRIVATE
  sourcemeta::core::uri)
