if(NOT BoostRegex_FOUND)
  set(BOOST_REGEX_DIR "${PROJECT_SOURCE_DIR}/vendor/boost-regex")
  set(BOOST_REGEX_PUBLIC_HEADER "${BOOST_REGEX_DIR}/include/boost/regex.hpp")

  set(BOOST_REGEX_PRIVATE_HEADERS
    "${BOOST_REGEX_DIR}/include/boost/cregex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex.h"
    "${BOOST_REGEX_DIR}/include/boost/regex_fwd.hpp")
  set(BOOST_REGEX_PRIVATE_HEADERS_REGEX
    "${BOOST_REGEX_DIR}/include/boost/regex/concepts.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/config.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/icu.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/mfc.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/pattern_except.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/user.hpp")
  set(BOOST_REGEX_PRIVATE_HEADERS_REGEX_CONFIG
    "${BOOST_REGEX_DIR}/include/boost/regex/config/borland.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/config/cwchar.hpp")
  set(BOOST_REGEX_PRIVATE_HEADERS_REGEX_PENDING
    "${BOOST_REGEX_DIR}/include/boost/regex/pending/object_cache.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/pending/static_mutex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/pending/unicode_iterator.hpp")
  set(BOOST_REGEX_PRIVATE_HEADERS_REGEX_V4
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/basic_regex_parser.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/object_cache.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/cregex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/basic_regex_creator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_format.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/u32regex_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/states.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/perl_matcher_recursive.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/perl_matcher.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_grep.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/match_flags.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/w32_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_replace.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/perl_matcher_common.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/c_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_traits_defaults.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/cpp_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/syntax_type.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/protected_call.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regbase.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_raw_buffer.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/basic_regex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/error_type.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/icu.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_match.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/u32regex_token_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_search.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/indexed_bit_flag.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_token_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/iterator_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/perl_matcher_non_recursive.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_workaround.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/sub_match.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_merge.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_fwd.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/iterator_category.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/regex_split.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/unicode_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/primary_transform.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/char_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/mem_block_cache.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/pattern_except.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v4/match_results.hpp")
  set(BOOST_REGEX_PRIVATE_HEADERS_REGEX_V5
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/basic_regex_parser.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/object_cache.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/cregex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/basic_regex_creator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_format.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/u32regex_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/states.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/perl_matcher.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_grep.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/match_flags.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/w32_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_replace.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/perl_matcher_common.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/c_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_traits_defaults.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/cpp_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/syntax_type.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regbase.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_raw_buffer.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/basic_regex.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/error_type.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/icu.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_match.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/u32regex_token_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_search.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_token_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/iterator_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/perl_matcher_non_recursive.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_workaround.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/sub_match.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_merge.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_fwd.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/iterator_category.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/regex_split.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/unicode_iterator.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/primary_transform.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/char_regex_traits.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/mem_block_cache.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/pattern_except.hpp"
    "${BOOST_REGEX_DIR}/include/boost/regex/v5/match_results.hpp")

  add_library(boost_regex INTERFACE
    "${BOOST_REGEX_PUBLIC_HEADER}"
    ${BOOST_REGEX_PRIVATE_HEADERS}
    ${BOOST_REGEX_PRIVATE_HEADERS_REGEX}
    ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_CONFIG}
    ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_PENDING}
    ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_V4}
    ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_V5})

  target_compile_definitions(boost_regex INTERFACE BOOST_REGEX_STANDALONE)

  if(SOURCEMETA_CORE_UNDEFINED_SANITIZER AND SOURCEMETA_COMPILER_LLVM)
    # Boost Regex doesn't pass the LLVM Undefined Behavior sanitizer otherwise
    # vendor/boost-regex/include/boost/regex/v5/cpp_regex_traits.hpp:1022:60:
    # runtime error: implicit conversion from type 'unsigned char' of value 128
    # (8-bit, unsigned) to type 'char' changed the value to -128 (8-bit, signed)
    target_compile_options(boost_regex INTERFACE -fno-sanitize=integer)
    target_compile_options(boost_regex INTERFACE -fno-sanitize=implicit-conversion)
  endif()

  add_library(Boost::regex ALIAS boost_regex)

  target_include_directories(boost_regex INTERFACE
    "$<BUILD_INTERFACE:${BOOST_REGEX_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  set_target_properties(boost_regex
    PROPERTIES
      OUTPUT_NAME boost_regex
      PUBLIC_HEADER "${BOOST_REGEX_PUBLIC_HEADER}"
      PRIVATE_HEADER "${BOOST_REGEX_PRIVATE_HEADERS}"
      EXPORT_NAME boost_regex)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS boost_regex
      EXPORT boost_regex
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost"
        COMPONENT sourcemeta_core
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost"
        COMPONENT sourcemeta_core
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(FILES ${BOOST_REGEX_PRIVATE_HEADERS_REGEX}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost/regex"
        COMPONENT sourcemeta_core)
    install(FILES ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_CONFIG}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost/regex/config"
        COMPONENT sourcemeta_core)
    install(FILES ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_PENDING}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost/regex/pending"
        COMPONENT sourcemeta_core)
    install(FILES ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_V4}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost/regex/v4"
        COMPONENT sourcemeta_core)
    install(FILES ${BOOST_REGEX_PRIVATE_HEADERS_REGEX_V5}
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/boost/regex/v5"
        COMPONENT sourcemeta_core)
    install(EXPORT boost_regex
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/boostregex"
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/boostregex-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/boost_regex.cmake\")\n"
      "check_required_components(\"boostregex\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/boostregex-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/boostregex"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(BoostRegex_FOUND ON)
endif()
