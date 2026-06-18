# Applies the shared compiler defaults to <target> at <visibility>.
#
# Flag categories handled here:
#   - Diagnostics (-W...): always on, every config
#   - Language semantics (-fwrapv, -fstrict-aliasing, -fno-rtti on GCC): always on
#   - Optimization-related (loop unrolling, vectorization, fast-math relaxations):
#     gated to non-Debug configs because they have no effect at -O0 but still
#     cost Clang/GCC pipeline time
#
# Do not add optimization-only flags here without a generator-expression gate
function(sourcemeta_add_default_options visibility target)
  if(SOURCEMETA_COMPILER_MSVC)
    # See https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-by-category
    target_compile_options("${target}" ${visibility}
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/options:strict>
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/permissive->
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/W4>
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/WL>
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/MP>
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/sdl>
      # See https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard
      $<$<OR:$<COMPILE_LANGUAGE:C>,$<COMPILE_LANGUAGE:CXX>>:/guard:cf>)
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
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:-Wnon-virtual-dtor>
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:-Woverloaded-virtual>
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:-Winvalid-offsetof>
      # Semantics, not optimization: keep on for every config
      -fstrict-aliasing
      # Assume that signed arithmetic overflow of addition, subtraction and
      # multiplication wraps around using twos-complement representation
      # See https://users.cs.utah.edu/~regehr/papers/overflow12.pdf
      # See https://www.postgresql.org/message-id/1689.1134422394@sss.pgh.pa.us
      -fwrapv
      # Fast-math relaxations relax IEEE conformance (errno after math.h,
      # signed-zero handling, reassociation), so they affect observable
      # behavior and must apply to every config to keep Debug and Release
      # semantics aligned
      -fno-math-errno
      -fno-trapping-math
      -fno-signed-zeros
      -freciprocal-math
      -fassociative-math

      # Optimization-only: emitted only when not building Debug. At -O0 these
      # run analyses that never reach codegen, costing build time for no
      # behavioral effect
      $<$<NOT:$<CONFIG:Debug>>:-funroll-loops>
      $<$<NOT:$<CONFIG:Debug>>:-ftree-vectorize>

      # See https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
      -Wformat
      -Wformat=2
      -Werror=format-security
      -fstrict-flex-arrays=3)

    # Hardware-assisted control-flow protection. The compiler emits these as
    # HINT-space instructions that are NOPs on CPUs without the feature, so
    # binaries remain compatible with older hardware
    if(SOURCEMETA_OS_LINUX)
      if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        target_compile_options("${target}" ${visibility} -fcf-protection=full)
      elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64"
          OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        target_compile_options("${target}" ${visibility} -mbranch-protection=standard)
      endif()
    endif()
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

      # Manage Objective-C and Objective-C++ object lifetimes with Automatic
      # Reference Counting
      $<$<OR:$<COMPILE_LANGUAGE:OBJC>,$<COMPILE_LANGUAGE:OBJCXX>>:-fobjc-arc>

      # Enable loop vectorization for performance reasons
      $<$<NOT:$<CONFIG:Debug>>:-fvectorize>
      # Enable vectorization of straight-line code for performance
      $<$<NOT:$<CONFIG:Debug>>:-fslp-vectorize>)

    # Prevent the compiler from deleting redundant null-pointer checks after
    # a dereference would normally prove them unreachable
    target_compile_options("${target}" ${visibility}
      $<$<NOT:$<CONFIG:Debug>>:-fno-delete-null-pointer-checks>)
  elseif(SOURCEMETA_COMPILER_GCC)
    target_compile_options("${target}" ${visibility}
      # Newer versions of GCC (i.e. 14) seem to print a lot of false-positives here
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:-Wno-dangling-reference>
      # GCC seems to print a lot of false-positives here
      -Wno-free-nonheap-object
      # Disables runtime type information
      $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:-fno-rtti>
      # See https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html
      -Wtrampolines
      -Wbidi-chars=any
      -fstack-clash-protection)

    # Prevent the compiler from deleting redundant null-pointer checks after
    # a dereference would normally prove them unreachable
    target_compile_options("${target}" ${visibility}
      $<$<NOT:$<CONFIG:Debug>>:-fno-delete-null-pointer-checks>)

    # Prevent the compiler from assuming shared library symbols could be
    # interposed at runtime, enabling more inlining and devirtualization
    if(BUILD_SHARED_LIBS)
      target_compile_options("${target}" ${visibility} -fno-semantic-interposition)
    endif()

    # _GLIBCXX_ASSERTIONS is libstdc++ (GNU) specific, not honored by libc++
    # (which the LLVM toolchain on Apple ships). Restrict to non-Apple GCC
    # to avoid emitting a Debug-only definition that does nothing on macOS
    if(NOT APPLE)
      target_compile_definitions("${target}" ${visibility}
        $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
    endif()
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
