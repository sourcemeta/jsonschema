# Get the list of languages defined in the project
get_property(SOURCEMETA_LANGUAGES GLOBAL PROPERTY ENABLED_LANGUAGES)

# Compiler detection (C++)
# TODO: Detect compilers on programming languages other than C++
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(SOURCEMETA_COMPILER_LLVM ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(SOURCEMETA_COMPILER_GCC ON)
elseif(MSVC)
  set(SOURCEMETA_COMPILER_MSVC ON)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(SOURCEMETA_OS_MACOS ON)
# It seems that in some cases, `LINUX` is not set on GNU/Linux on WSL
elseif(LINUX OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(SOURCEMETA_OS_LINUX ON)
elseif(WIN32)
  set(SOURCEMETA_OS_WINDOWS ON)
elseif(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD")
  set(SOURCEMETA_OS_BSD ON)
endif()
