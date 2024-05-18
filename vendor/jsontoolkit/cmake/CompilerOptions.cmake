function(sourcemeta_jsontoolkit_add_compile_options target)
  if(NOA_COMPILER_MSVC)
    # See https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category
    target_compile_options("${target}" PRIVATE
      /options:strict
      /permissive-
      /W4
      /WL
      /MP
      /sdl)
  else()
    target_compile_options("${target}" PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Wshadow
      -Wdouble-promotion
      -Wconversion
      -Wunused-parameter
      -Wtrigraphs
      -Wunreachable-code
      -Wmissing-braces
      -Wparentheses
      -Wswitch
      -Wunused-function
      -Wunused-label
      -Wunused-parameter
      -Wunused-variable
      -Wunused-value
      -Wempty-body
      -Wuninitialized
      -Wshadow
      -Wconversion
      -Wenum-conversion
      -Wfloat-conversion
      -Wimplicit-fallthrough
      -Wsign-compare
      -Wsign-conversion
      -Wunknown-pragmas
      -Wnon-virtual-dtor
      -Woverloaded-virtual
      -Winvalid-offsetof)
  endif()

  if(NOA_COMPILER_LLVM)
    target_compile_options("${target}" PRIVATE
      -Wbool-conversion
      -Wint-conversion
      -Wpointer-sign
      -Wconditional-uninitialized
      -Wconstant-conversion
      -Wnon-literal-null-conversion
      -Wshorten-64-to-32
      -Wdeprecated-implementations
      -Winfinite-recursion
      -Wnewline-eof
      -Wfour-char-constants
      -Wselector
      -Wundeclared-selector
      -Wdocumentation
      -Wmove
      -Wc++11-extensions
      -Wno-exit-time-destructors
      -Wrange-loop-analysis)
  endif()
endfunction()
