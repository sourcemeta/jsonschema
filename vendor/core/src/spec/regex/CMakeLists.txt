sourcemeta_library(NAMESPACE sourcemeta PROJECT core NAME regex
  SOURCES regex.cc ecma262.h ecma262_properties.h iregexp.h permissive.h)

if(SOURCEMETA_CORE_INSTALL)
  sourcemeta_library_install(NAMESPACE sourcemeta PROJECT core NAME regex)
endif()

# An object library hands its objects to the targets that name it, and a
# second object library in between only passes on usage requirements, so the
# code generator has to be named here rather than left to the library that
# actually calls into it
target_link_libraries(sourcemeta_core_regex PRIVATE SLJIT::sljit)
target_link_libraries(sourcemeta_core_regex PRIVATE PCRE2::pcre2)
target_link_libraries(sourcemeta_core_regex PRIVATE sourcemeta::core::text)
target_link_libraries(sourcemeta_core_regex PRIVATE sourcemeta::core::unicode)
