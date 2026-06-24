function(sourcemeta_clang_tidy_attempt_install)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL "" "OUTPUT_DIRECTORY" "" ${ARGN})
  if(NOT SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "You must pass the output directory in the OUTPUT_DIRECTORY option")
  endif()

  # See https://pypi.org/project/clang-tidy/
  set(CLANG_TIDY_BINARY_VERSION "22.1.7")
  set(CLANG_TIDY_BINARY_Windows_AMD64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_TIDY_BINARY_MSYS_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_TIDY_BINARY_Darwin_arm64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-macosx_11_0_arm64.whl")
  set(CLANG_TIDY_BINARY_Darwin_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-macosx_10_9_x86_64.whl")
  set(CLANG_TIDY_BINARY_Linux_aarch64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-manylinux_2_26_aarch64.manylinux_2_28_aarch64.whl")
  set(CLANG_TIDY_BINARY_Linux_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-manylinux_2_27_x86_64.manylinux_2_28_x86_64.whl")
  set(CLANG_TIDY_BINARY_CHECKSUM_Windows_AMD64 "51/9b/755d77e51e8aebd03e287dc19727d5c4ca286fd419ff4cf5bd904f7b0366")
  set(CLANG_TIDY_BINARY_CHECKSUM_MSYS_x86_64 "51/9b/755d77e51e8aebd03e287dc19727d5c4ca286fd419ff4cf5bd904f7b0366")
  set(CLANG_TIDY_BINARY_CHECKSUM_Darwin_arm64 "e2/eb/bd4179187eb12348350d9de3a3124fb1466f0fd5ff33619ed45575752683")
  set(CLANG_TIDY_BINARY_CHECKSUM_Darwin_x86_64 "bc/4e/de59bc2bda314fa0622b967d0542e16c78e156038827422740fc9749a3b0")
  set(CLANG_TIDY_BINARY_CHECKSUM_Linux_aarch64 "27/8d/f21976010dfd1746d5eac80bccaa364f56df61ab968c6d9279513e21f945")
  set(CLANG_TIDY_BINARY_CHECKSUM_Linux_x86_64 "43/69/3be98747ee4e8aedcfc4525a8e0c1576118bcd4de9a560d84a2a398158ce")
  set(CLANG_TIDY_BINARY_NAME_Windows_AMD64 "clang-tidy.exe")
  set(CLANG_TIDY_BINARY_NAME_MSYS_x86_64 "clang-tidy.exe")
  set(CLANG_TIDY_BINARY_NAME_Darwin_arm64 "clang-tidy")
  set(CLANG_TIDY_BINARY_NAME_Darwin_x86_64 "clang-tidy")
  set(CLANG_TIDY_BINARY_NAME_Linux_aarch64 "clang-tidy")
  set(CLANG_TIDY_BINARY_NAME_Linux_x86_64 "clang-tidy")

  # Determine the pre-built binary URL
  string(REPLACE "." "_" CLANG_TIDY_BINARY_SYSTEM "${CMAKE_SYSTEM_NAME}")
  string(REPLACE "." "_" CLANG_TIDY_BINARY_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
  set(CLANG_TIDY_BINARY_URL_VAR "CLANG_TIDY_BINARY_${CLANG_TIDY_BINARY_SYSTEM}_${CLANG_TIDY_BINARY_ARCH}")
  set(CLANG_TIDY_BINARY_CHECKSUM_VAR "CLANG_TIDY_BINARY_CHECKSUM_${CLANG_TIDY_BINARY_SYSTEM}_${CLANG_TIDY_BINARY_ARCH}")
  set(CLANG_TIDY_BINARY_NAME_VAR "CLANG_TIDY_BINARY_NAME_${CLANG_TIDY_BINARY_SYSTEM}_${CLANG_TIDY_BINARY_ARCH}")
  if(NOT DEFINED ${CLANG_TIDY_BINARY_URL_VAR} OR "${${CLANG_TIDY_BINARY_URL_VAR}}" STREQUAL "")
    message(WARNING "Skipping `clang-tidy` download. No known pre-build binary URL")
    return()
  elseif(NOT DEFINED ${CLANG_TIDY_BINARY_CHECKSUM_VAR} OR "${${CLANG_TIDY_BINARY_CHECKSUM_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `clang-tidy` pre-build binary checksum")
  elseif(NOT DEFINED ${CLANG_TIDY_BINARY_NAME_VAR} OR "${${CLANG_TIDY_BINARY_NAME_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `clang-tidy` pre-build binary name")
  endif()
  set(CLANG_TIDY_BINARY_URL "https://files.pythonhosted.org/packages/${${CLANG_TIDY_BINARY_CHECKSUM_VAR}}/${${CLANG_TIDY_BINARY_URL_VAR}}")

  # Download and extract the pre-built binary ZIP if needed
  set(CLANG_TIDY_BINARY_NAME "${${CLANG_TIDY_BINARY_NAME_VAR}}")
  set(CLANG_TIDY_BINARY_OUTPUT "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}/${CLANG_TIDY_BINARY_NAME}")
  get_filename_component(CLANG_TIDY_BINARY_ROOT
      "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}" DIRECTORY)
  set(CLANG_TIDY_BINARY_RESOURCE_DIRECTORY "${CLANG_TIDY_BINARY_ROOT}/lib/clang")
  if(EXISTS "${CLANG_TIDY_BINARY_OUTPUT}" AND IS_DIRECTORY "${CLANG_TIDY_BINARY_RESOURCE_DIRECTORY}")
    message(STATUS "Found existing `clang-tidy` pre-built binary at ${CLANG_TIDY_BINARY_OUTPUT}")
    return()
  endif()
  set(CLANG_TIDY_BINARY_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/clang-tidy")
  file(REMOVE_RECURSE "${CLANG_TIDY_BINARY_DOWNLOAD_DIR}")
  file(MAKE_DIRECTORY "${CLANG_TIDY_BINARY_DOWNLOAD_DIR}")
  set(CLANG_TIDY_BINARY_WHEEL "${CLANG_TIDY_BINARY_DOWNLOAD_DIR}/clang-tidy.whl")
  message(STATUS "Downloading `clang-tidy` pre-built binary from ${CLANG_TIDY_BINARY_URL}")
  file(DOWNLOAD "${CLANG_TIDY_BINARY_URL}" "${CLANG_TIDY_BINARY_WHEEL}"
    STATUS CLANG_TIDY_BINARY_DOWNLOAD_STATUS SHOW_PROGRESS TLS_VERIFY ON
    LOG CLANG_TIDY_BINARY_DOWNLOAD_LOG)
  list(GET CLANG_TIDY_BINARY_DOWNLOAD_STATUS 0 _code)
  if(NOT _code EQUAL 0)
    message(WARNING "Failed to download the `clang-tidy` pre-built binary")
    message(WARNING "${CLANG_TIDY_BINARY_DOWNLOAD_LOG}")
    file(REMOVE_RECURSE "${CLANG_TIDY_BINARY_DOWNLOAD_DIR}")
    return()
  endif()
  set(CLANG_TIDY_BINARY_EXTRACT_DIR "${CLANG_TIDY_BINARY_DOWNLOAD_DIR}/extracted")
  file(MAKE_DIRECTORY "${CLANG_TIDY_BINARY_EXTRACT_DIR}")
  file(ARCHIVE_EXTRACT INPUT "${CLANG_TIDY_BINARY_WHEEL}" DESTINATION "${CLANG_TIDY_BINARY_EXTRACT_DIR}")

  # Install the binary alongside the resource directory that ships in the
  # wheel, replicating the upstream `bin` and `lib` layout so that clang-tidy
  # discovers its own matching built-in headers. Otherwise it falls back to the
  # system compiler resource directory, which breaks whenever that compiler is
  # newer than the clang-tidy front-end
  file(MAKE_DIRECTORY "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(COPY "${CLANG_TIDY_BINARY_EXTRACT_DIR}/clang_tidy/data/bin/${CLANG_TIDY_BINARY_NAME}"
       DESTINATION "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(CHMOD "${CLANG_TIDY_BINARY_OUTPUT}" PERMISSIONS
       OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  file(COPY "${CLANG_TIDY_BINARY_EXTRACT_DIR}/clang_tidy/data/lib"
       DESTINATION "${CLANG_TIDY_BINARY_ROOT}")
  message(STATUS "Installed `clang-tidy` pre-built binary to ${CLANG_TIDY_BINARY_OUTPUT}")
endfunction()

function(sourcemeta_clang_tidy_attempt_enable)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_ENABLE "" "TARGET" "" ${ARGN})
  if(NOT SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_ENABLE_TARGET)
    message(FATAL_ERROR "You must pass the target name using the TARGET option")
  endif()

  # TODO: Support other platforms too, like Linux
  if(APPLE AND SOURCEMETA_COMPILER_LLVM)
    message(STATUS "Enabling ClangTidy alongside compilation for target ${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_ENABLE_TARGET}")
  else()
    return()
  endif()

  # We rely on this cache variable to not pre-compute the ClangTidy
  # setup over and over again for every single target
  if(NOT SOURCEMETA_CXX_CLANG_TIDY)
    sourcemeta_clang_tidy_attempt_install(
      OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
    find_program(CLANG_TIDY_BIN NAMES clang-tidy
        NO_DEFAULT_PATH
        PATHS "${PROJECT_BINARY_DIR}/bin"
        REQUIRED)

    set(CLANG_TIDY_CONFIG "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/clang-tidy.json")
    execute_process(COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOSX_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(SOURCEMETA_CXX_CLANG_TIDY
        "${CLANG_TIDY_BIN};--config-file=${CLANG_TIDY_CONFIG};-header-filter=${PROJECT_SOURCE_DIR}/src/*"
        "--extra-arg=-isysroot"
        "--extra-arg=${MACOSX_SDK_PATH}"
        CACHE STRING "CXX_CLANG_TIDY")
  endif()

  set_target_properties("${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_ENABLE_TARGET}"
    PROPERTIES CXX_CLANG_TIDY "${SOURCEMETA_CXX_CLANG_TIDY}")
endfunction()
