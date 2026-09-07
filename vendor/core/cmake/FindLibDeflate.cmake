if(NOT LibDeflate_FOUND)
  set(LIBDEFLATE_DIR "${PROJECT_SOURCE_DIR}/vendor/libdeflate")
  set(LIBDEFLATE_LIB_DIR "${LIBDEFLATE_DIR}/lib")

  set(LIBDEFLATE_SOURCES
    "${LIBDEFLATE_LIB_DIR}/utils.c"
    "${LIBDEFLATE_LIB_DIR}/deflate_compress.c"
    "${LIBDEFLATE_LIB_DIR}/deflate_decompress.c"
    "${LIBDEFLATE_LIB_DIR}/gzip_compress.c"
    "${LIBDEFLATE_LIB_DIR}/gzip_decompress.c"
    "${LIBDEFLATE_LIB_DIR}/adler32.c"
    "${LIBDEFLATE_LIB_DIR}/crc32.c"
    "${LIBDEFLATE_LIB_DIR}/zlib_compress.c"
    "${LIBDEFLATE_LIB_DIR}/zlib_decompress.c")

  # Platform-specific CPU feature detection
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    list(APPEND LIBDEFLATE_SOURCES
      "${LIBDEFLATE_LIB_DIR}/arm/cpu_features.c")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|x86|i[3-6]86")
    list(APPEND LIBDEFLATE_SOURCES
      "${LIBDEFLATE_LIB_DIR}/x86/cpu_features.c")
  endif()

  # Merged into the library that uses it, so that no archive, no header and
  # no CMake package of our own build of it reaches an installed consumer
  add_library(libdeflate OBJECT ${LIBDEFLATE_SOURCES})
  sourcemeta_add_default_options(PRIVATE libdeflate)

  # Check if the assembler supports ARM dot-product (udot) instructions.
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    include(CheckCSourceCompiles)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND
        CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
      check_c_source_compiles("
        #include <arm_neon.h>
        __attribute__((target(\"+dotprod\")))
        int test(void) {
          uint32x4_t a = vdupq_n_u32(0);
          uint8x16_t b = vdupq_n_u8(0);
          uint8x16_t c = vdupq_n_u8(0);
          a = vdotq_u32(a, b, c);
          return (int)vgetq_lane_u32(a, 0);
        }
        int main(void) { return test(); }
      " LIBDEFLATE_HAS_DOTPROD_ASSEMBLER)
    else()
      set(LIBDEFLATE_SAVED_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
      set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=armv8.2-a+dotprod")
      check_c_source_compiles("
        #include <arm_neon.h>
        int main(void) {
          uint32x4_t a = vdupq_n_u32(0);
          uint8x16_t b = vdupq_n_u8(0);
          uint8x16_t c = vdupq_n_u8(0);
          a = vdotq_u32(a, b, c);
          return 0;
        }
      " LIBDEFLATE_HAS_DOTPROD_ASSEMBLER)
      set(CMAKE_REQUIRED_FLAGS "${LIBDEFLATE_SAVED_CMAKE_REQUIRED_FLAGS}")
    endif()
    if(NOT LIBDEFLATE_HAS_DOTPROD_ASSEMBLER)
      target_compile_definitions(libdeflate PRIVATE
        LIBDEFLATE_ASSEMBLER_DOES_NOT_SUPPORT_DOTPROD)
    endif()

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND
        CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
      check_c_source_compiles("
        #include <arm_neon.h>
        __attribute__((target(\"+crypto,+crc,+sha3\")))
        int test(void) {
          uint8x16_t a = vdupq_n_u8(0);
          uint8x16_t b = vdupq_n_u8(0);
          uint8x16_t c = vdupq_n_u8(0);
          a = veor3q_u8(a, b, c);
          return (int)vgetq_lane_u8(a, 0);
        }
        int main(void) { return test(); }
      " LIBDEFLATE_HAS_SHA3_ASSEMBLER)
    else()
      set(LIBDEFLATE_SAVED_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
      set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -march=armv8.2-a+crypto+crc+sha3")
      check_c_source_compiles("
        #include <arm_neon.h>
        int main(void) {
          uint8x16_t a = vdupq_n_u8(0);
          uint8x16_t b = vdupq_n_u8(0);
          uint8x16_t c = vdupq_n_u8(0);
          a = veor3q_u8(a, b, c);
          return 0;
        }
      " LIBDEFLATE_HAS_SHA3_ASSEMBLER)
      set(CMAKE_REQUIRED_FLAGS "${LIBDEFLATE_SAVED_CMAKE_REQUIRED_FLAGS}")
    endif()
    if(NOT LIBDEFLATE_HAS_SHA3_ASSEMBLER)
      target_compile_definitions(libdeflate PRIVATE
        LIBDEFLATE_ASSEMBLER_DOES_NOT_SUPPORT_SHA3)
    endif()
  endif()

  target_include_directories(libdeflate PUBLIC
    "$<BUILD_INTERFACE:${LIBDEFLATE_DIR}>")

  target_include_directories(libdeflate PRIVATE
    "${LIBDEFLATE_LIB_DIR}")

  # Marking every entry point as visible would publish this library from the
  # one it is merged into, where it can collide with a real installation of it
  target_compile_definitions(libdeflate PRIVATE LIBDEFLATE_EXPORT_SYM=)

  # The processor feature detection this library performs on Linux reaches for
  # interfaces that the C library only declares outside the strict ISO mode
  # this project otherwise compiles C in
  if(SOURCEMETA_OS_LINUX)
    target_compile_definitions(libdeflate PRIVATE _GNU_SOURCE)
  endif()

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(libdeflate PRIVATE -Wno-conversion)
    target_compile_options(libdeflate PRIVATE -Wno-sign-conversion)
    target_compile_options(libdeflate PRIVATE -Wno-sign-compare)
    target_compile_options(libdeflate PRIVATE -Wno-implicit-int-conversion)
    target_compile_options(libdeflate PRIVATE -Wno-shorten-64-to-32)
    target_compile_options(libdeflate PRIVATE -Wno-unused-parameter)
  endif()

  if(SOURCEMETA_COMPILER_MSVC)
    target_compile_options(libdeflate PRIVATE /wd4113)
    target_compile_options(libdeflate PRIVATE /wd4244)
    target_compile_options(libdeflate PRIVATE /wd4267)
  endif()

  add_library(LibDeflate::LibDeflate ALIAS libdeflate)

  set(LibDeflate_FOUND ON)
endif()
