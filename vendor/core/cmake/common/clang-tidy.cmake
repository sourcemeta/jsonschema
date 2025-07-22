function(sourcemeta_clang_tidy_attempt_install)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL "" "OUTPUT_DIRECTORY" "" ${ARGN})
  if(NOT SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "You must pass the output directory in the OUTPUT_DIRECTORY option")
  endif()

  # See https://pypi.org/project/clang-tidy/
  set(CLANG_TIDY_BINARY_VERSION "20.1.0")
  set(CLANG_TIDY_BINARY_Windows_AMD64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_TIDY_BINARY_MSYS_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_TIDY_BINARY_Darwin_arm64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-macosx_11_0_arm64.whl")
  set(CLANG_TIDY_BINARY_Darwin_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-macosx_10_9_x86_64.whl")
  set(CLANG_TIDY_BINARY_Linux_aarch64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-manylinux_2_17_aarch64.manylinux2014_aarch64.whl")
  set(CLANG_TIDY_BINARY_Linux_x86_64 "clang_tidy-${CLANG_TIDY_BINARY_VERSION}-py2.py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl")
  set(CLANG_TIDY_BINARY_CHECKSUM_Windows_AMD64 "02/f0/dd985d9d9b76f8c39f1995aa475d8d5aabbea0d3e0cf498df44dc7bf1cb0")
  set(CLANG_TIDY_BINARY_CHECKSUM_MSYS_x86_64 "02/f0/dd985d9d9b76f8c39f1995aa475d8d5aabbea0d3e0cf498df44dc7bf1cb0")
  set(CLANG_TIDY_BINARY_CHECKSUM_Darwin_arm64 "95/02/838baf08764b08327322096bda55e8d1e2344e4a13b9308e5642cfaafd8e")
  set(CLANG_TIDY_BINARY_CHECKSUM_Darwin_x86_64 "6d/5b/dcfc84b895d8544e00186738ca85132bbd14db4d11dbe39502630ece5391")
  set(CLANG_TIDY_BINARY_CHECKSUM_Linux_aarch64 "be/61/9e1a0797639e81c41d38d7b8b2508a9be4b05b9a23baa9d64e7284d07238")
  set(CLANG_TIDY_BINARY_CHECKSUM_Linux_x86_64 "52/76/42c61be1c1fdf8bacdbb265f0cd3e11321fee7362f91fa840717a6a41ad6")
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
  if(EXISTS "${CLANG_TIDY_BINARY_OUTPUT}")
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

  # Install the binary
  file(MAKE_DIRECTORY "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(COPY "${CLANG_TIDY_BINARY_EXTRACT_DIR}/clang_tidy/data/bin/${CLANG_TIDY_BINARY_NAME}"
       DESTINATION "${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(CHMOD "${CLANG_TIDY_BINARY_OUTPUT}" PERMISSIONS
       OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
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
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir
        OUTPUT_VARIABLE MACOSX_RESOURCE_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(SOURCEMETA_CXX_CLANG_TIDY
        "${CLANG_TIDY_BIN};--config-file=${CLANG_TIDY_CONFIG};-header-filter=${PROJECT_SOURCE_DIR}/src/*"
        "--extra-arg=-isysroot"
        "--extra-arg=${MACOSX_SDK_PATH}"
        "--extra-arg=-resource-dir=${MACOSX_RESOURCE_PATH}"
        CACHE STRING "CXX_CLANG_TIDY")
  endif()

  set_target_properties("${SOURCEMETA_TARGET_CLANG_TIDY_ATTEMPT_ENABLE_TARGET}"
    PROPERTIES CXX_CLANG_TIDY "${SOURCEMETA_CXX_CLANG_TIDY}")
endfunction()
