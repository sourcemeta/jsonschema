# Standards (sane modern defaults)
if("CXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_CXX_STANDARD 20)
endif()
if("C" IN_LIST NOA_LANGUAGES)
  set(CMAKE_C_STANDARD 11)
endif()
if("OBJCXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_OBJCXX_STANDARD "${CMAKE_CXX_STANDARD}")
endif()

# Hide symbols from shared libraries by default
# In certain compilers, like GCC and Clang,
# symbols are visible by default.
set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
if("CXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()
if("C" IN_LIST NOA_LANGUAGES)
  set(CMAKE_C_VISIBILITY_PRESET hidden)
endif()
if("OBJCXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
endif()

# By default, stay within ISO C++
if("CXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()
if("C" IN_LIST NOA_LANGUAGES)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  set(CMAKE_C_EXTENSIONS OFF)
endif()
if("OBJCXX" IN_LIST NOA_LANGUAGES)
  set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
  set(CMAKE_OBJCXX_EXTENSIONS OFF)
endif()

# Export compile commands by default.
# It is very useful for IDE integration, linting, etc
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Prevent DT_RPATH/DT_RUNPATH problem
# This problem is not present on Apple platforms.
# See https://www.youtube.com/watch?v=m0DwB4OvDXk
if(NOT APPLE)
  set(CMAKE_INSTALL_RPATH $ORIGIN)
endif()

# Delay GoogleTest discovery until before running the tests
# See https://discourse.cmake.org/t/default-value-for-new-discovery-mode-option-for-gtest-discover-tests/1422
set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

# Always use folders in IDE
# See https://cmake.org/cmake/help/latest/prop_gbl/USE_FOLDERS.html
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# On Windows, during build, put executables and libraries in the same directory.
# Otherwise, if there is any shared library being generated, the binaries
# linking to it will not be able to find it and i.e. unit tests will fail.
# Note that GoogleTest does this already to a non-configurable top-level
# `bin` directory, so adopting that convention here.
# See https://stackoverflow.com/q/39807664
# See https://github.com/google/googletest/blob/e47544ad31cb3ceecd04cc13e8fe556f8df9fe0b/googletest/cmake/internal_utils.cmake#L173-L174
if(WIN32)
  # For EXE files
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE STRING "")
  # For DLL files
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" CACHE STRING "")
endif()

# Enable IPO/LTO to help the compiler optimize across modules.
# Only do so in release, given these optimizations can significantly
# increase build times.

# Note we don't use CheckIPOSupported and CMAKE_INTERPROCEDURAL_OPTIMIZATION,
# as those CMake features can only enable thin LTO. For Fat LTO, see:
# - https://gcc.gnu.org/onlinedocs/gccint/LTO-Overview.html
# - https://llvm.org/docs/FatLTO.html

if(NOA_COMPILER_GCC AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT BUILD_SHARED_LIBS)
  message(STATUS "Enabling Fat LTO")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto -ffat-lto-objects")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
endif()

# TODO: Make this work on Linux on LLVM
if(NOA_COMPILER_LLVM AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT BUILD_SHARED_LIBS AND APPLE)
  message(STATUS "Enabling Fat LTO")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=full")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto=full")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto=full")
endif()
