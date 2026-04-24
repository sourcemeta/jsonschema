if(NOT LibDeflate_FOUND)
  set(LIBDEFLATE_DIR "${PROJECT_SOURCE_DIR}/vendor/libdeflate")
  set(LIBDEFLATE_LIB_DIR "${LIBDEFLATE_DIR}/lib")
  set(LIBDEFLATE_PUBLIC_HEADER "${LIBDEFLATE_DIR}/libdeflate.h")

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

  add_library(libdeflate STATIC ${LIBDEFLATE_SOURCES})
  sourcemeta_add_default_options(PRIVATE libdeflate)

  # Check if the assembler supports ARM dot-product (udot) instructions.
  # GCC 14+ assumes binutils is new enough, but some CI environments
  # pair GCC 14 with older binutils that lack udot support
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    include(CheckCSourceCompiles)
    # GCC >= 14 compiles dotprod code using a per-function target
    # attribute ("+dotprod") rather than a global -march flag.
    # This relies on binutils >= 2.41 to handle the resulting
    # .arch_extension directive. Test the same mechanism the code
    # actually uses so we detect old assemblers correctly.
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
  endif()

  target_include_directories(libdeflate PUBLIC
    "$<BUILD_INTERFACE:${LIBDEFLATE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_include_directories(libdeflate PRIVATE
    "${LIBDEFLATE_LIB_DIR}")

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

  set_target_properties(libdeflate
    PROPERTIES
      OUTPUT_NAME deflate
      PUBLIC_HEADER "${LIBDEFLATE_PUBLIC_HEADER}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      EXPORT_NAME LibDeflate)

  add_library(LibDeflate::LibDeflate ALIAS libdeflate)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS libdeflate
      EXPORT libdeflate
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT libdeflate
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdeflate"
      NAMESPACE LibDeflate::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libdeflate-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/libdeflate.cmake\")\n"
      "check_required_components(\"libdeflate\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/libdeflate-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdeflate"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(LibDeflate_FOUND ON)
endif()
