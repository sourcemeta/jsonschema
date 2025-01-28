function(sourcemeta_sanitizer)
  cmake_parse_arguments(SOURCEMETA_SANITIZER "" "TYPE" "" ${ARGN})

  if(NOT SOURCEMETA_SANITIZER_TYPE)
    message(FATAL_ERROR "You must pass the intended sanitizer")
  endif()

  if(SOURCEMETA_COMPILER_LLVM AND "${SOURCEMETA_SANITIZER_TYPE}" STREQUAL "address")
    # See https://clang.llvm.org/docs/AddressSanitizer.html
    message(STATUS "Enabling sanitizer: Clang AddressSanitizer")
    add_compile_options(-fsanitize=address -fsanitize-address-use-after-scope)
    add_link_options(-fsanitize=address)
    # Get nicer stack traces with the Address sanitizer
    add_compile_options(-fno-omit-frame-pointer -fno-optimize-sibling-calls)
    add_compile_options(-O1)
  elseif(SOURCEMETA_COMPILER_LLVM AND "${SOURCEMETA_SANITIZER_TYPE}" STREQUAL "memory")
    if(APPLE)
      message(FATAL_ERROR "Clang MemorySanitizer is not available on Apple platforms")
    endif()

    # See https://clang.llvm.org/docs/MemorySanitizer.html
    message(STATUS "Enabling sanitizer: Clang MemorySanitizer")
    add_compile_options(-fsanitize=memory -fno-sanitize-memory-use-after-dtor)
    add_link_options(-fsanitize=memory)
    # Get nicer stack traces with the Memory sanitizer
    add_compile_options(-fno-omit-frame-pointer -fno-optimize-sibling-calls)
    add_compile_options(-O1)
  elseif(SOURCEMETA_COMPILER_LLVM AND "${SOURCEMETA_SANITIZER_TYPE}" STREQUAL "undefined")
    # See https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    message(STATUS "Enabling sanitizer: Clang UndefinedBehaviorSanitizer")
    add_compile_options(-fsanitize=undefined,nullability,integer,implicit-conversion,local-bounds
      -fno-sanitize=unsigned-integer-overflow)
    add_link_options(-fsanitize=undefined,nullability,integer,implicit-conversion,local-bounds
      -fno-sanitize=unsigned-integer-overflow)
    # Exit after an error, otherwise this sanitizer only prints warnings
    add_compile_options(-fno-sanitize-recover=all)
  else()
    message(FATAL_ERROR "Unrecognized compiler and/or sanitizer combination")
  endif()
endfunction()
