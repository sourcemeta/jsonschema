function(sourcemeta_add_default_options visibility target)
  if(SOURCEMETA_COMPILER_MSVC)
    # See https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category
    target_compile_options("${target}" ${visibility}
      /options:strict
      /permissive-
      /W4
      /WL
      /MP
      /sdl)
  elseif(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options("${target}" ${visibility}
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
      -Winvalid-offsetof
      -funroll-loops
      -fstrict-aliasing
      -ftree-vectorize

      # To improve how much GCC/Clang will vectorize
      -fno-math-errno

      # Assume that signed arithmetic overflow of addition, subtraction and
      # multiplication wraps around using twos-complement representation
      # See https://users.cs.utah.edu/~regehr/papers/overflow12.pdf
      # See https://www.postgresql.org/message-id/1689.1134422394@sss.pgh.pa.us
      -fwrapv)
  endif()

  if(SOURCEMETA_COMPILER_LLVM)
    target_compile_options("${target}" ${visibility}
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
      -Wcomma
      -Wno-exit-time-destructors
      -Wrange-loop-analysis

      # Enable loop vectorization for performance reasons
      -fvectorize
      # Enable vectorization of straight-line code for performance
      -fslp-vectorize)
  elseif(SOURCEMETA_COMPILER_GCC)
    target_compile_options("${target}" ${visibility}
      -fno-trapping-math
      # Newer versions of GCC (i.e. 14) seem to print a lot of false-positives here
      -Wno-dangling-reference
      # GCC seems to print a lot of false-positives here
      -Wno-free-nonheap-object
      # Disables runtime type information
      -fno-rtti)
  endif()
endfunction()

# For studying failed vectorization results
# - On Clang , seems to only take effect on release shared builds
# - On GCC, seems to only take effect on release shared builds
function(sourcemeta_add_vectorization_diagnostics target)
  if(SOURCEMETA_COMPILER_LLVM)
    # See https://llvm.org/docs/Vectorizers.html#id6
    target_compile_options("${target}" PRIVATE
      -Rpass-analysis=loop-vectorize
      -Rpass-missed=loop-vectorize)
  elseif(SOURCEMETA_COMPILER_GCC)
    target_compile_options("${target}" PRIVATE
      -fopt-info-vec-missed
      -fopt-info-loop-missed)
  endif()
endfunction()
