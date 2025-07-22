function(sourcemeta_target_clang_format_attempt_install)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_FORMAT_ATTEMPT_INSTALL "" "OUTPUT_DIRECTORY" "" ${ARGN})
  if(NOT SOURCEMETA_TARGET_CLANG_FORMAT_ATTEMPT_INSTALL_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "You must pass the output directory in the OUTPUT_DIRECTORY option")
  endif()

  # See https://pypi.org/project/clang-format/
  set(CLANG_FORMAT_BINARY_VERSION "20.1.6")
  set(CLANG_FORMAT_BINARY_Windows_AMD64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_FORMAT_BINARY_MSYS_x86_64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-win_amd64.whl")
  set(CLANG_FORMAT_BINARY_Darwin_arm64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-macosx_11_0_arm64.whl")
  set(CLANG_FORMAT_BINARY_Darwin_x86_64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-macosx_10_9_x86_64.whl")
  set(CLANG_FORMAT_BINARY_Linux_aarch64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-manylinux_2_17_aarch64.manylinux2014_aarch64.whl")
  set(CLANG_FORMAT_BINARY_Linux_x86_64 "clang_format-${CLANG_FORMAT_BINARY_VERSION}-py2.py3-none-manylinux_2_17_x86_64.manylinux2014_x86_64.whl")
  set(CLANG_FORMAT_BINARY_CHECKSUM_Windows_AMD64 "76/d0/2781f7699ce9ff1f5f9035d30cdb4c46f40b6acf191e0100543c289f46be")
  set(CLANG_FORMAT_BINARY_CHECKSUM_MSYS_x86_64 "76/d0/2781f7699ce9ff1f5f9035d30cdb4c46f40b6acf191e0100543c289f46be")
  set(CLANG_FORMAT_BINARY_CHECKSUM_Darwin_arm64 "fd/27/171dcef3288369bc0f7034307cebc6ea5d9a2b03d44e5cfa5a218f0e4f53")
  set(CLANG_FORMAT_BINARY_CHECKSUM_Darwin_x86_64 "ac/f7/01502ff0869985df8b47ae62cdace425f02dfcd61b463a046f873ad5d2e2")
  set(CLANG_FORMAT_BINARY_CHECKSUM_Linux_aarch64 "b1/51/2a0f401f5a5e27f97b8ebfed6ca9c4ccc2809cabafa2f97c7ac8e5b0d882")
  set(CLANG_FORMAT_BINARY_CHECKSUM_Linux_x86_64 "b9/5e/7713e11945fa8018589e37a60052a1b1a2485be2292fcf382d154231eab6")
  set(CLANG_FORMAT_BINARY_NAME_Windows_AMD64 "clang-format.exe")
  set(CLANG_FORMAT_BINARY_NAME_MSYS_x86_64 "clang-format.exe")
  set(CLANG_FORMAT_BINARY_NAME_Darwin_arm64 "clang-format")
  set(CLANG_FORMAT_BINARY_NAME_Darwin_x86_64 "clang-format")
  set(CLANG_FORMAT_BINARY_NAME_Linux_aarch64 "clang-format")
  set(CLANG_FORMAT_BINARY_NAME_Linux_x86_64 "clang-format")

  # Determine the pre-built binary URL
  string(REPLACE "." "_" CLANG_FORMAT_BINARY_SYSTEM "${CMAKE_SYSTEM_NAME}")
  string(REPLACE "." "_" CLANG_FORMAT_BINARY_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
  set(CLANG_FORMAT_BINARY_URL_VAR "CLANG_FORMAT_BINARY_${CLANG_FORMAT_BINARY_SYSTEM}_${CLANG_FORMAT_BINARY_ARCH}")
  set(CLANG_FORMAT_BINARY_CHECKSUM_VAR "CLANG_FORMAT_BINARY_CHECKSUM_${CLANG_FORMAT_BINARY_SYSTEM}_${CLANG_FORMAT_BINARY_ARCH}")
  set(CLANG_FORMAT_BINARY_NAME_VAR "CLANG_FORMAT_BINARY_NAME_${CLANG_FORMAT_BINARY_SYSTEM}_${CLANG_FORMAT_BINARY_ARCH}")
  if(NOT DEFINED ${CLANG_FORMAT_BINARY_URL_VAR} OR "${${CLANG_FORMAT_BINARY_URL_VAR}}" STREQUAL "")
    message(WARNING "Skipping `clang-format` download. No known pre-build binary URL")
    return()
  elseif(NOT DEFINED ${CLANG_FORMAT_BINARY_CHECKSUM_VAR} OR "${${CLANG_FORMAT_BINARY_CHECKSUM_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `clang-format` pre-build binary checksum")
  elseif(NOT DEFINED ${CLANG_FORMAT_BINARY_NAME_VAR} OR "${${CLANG_FORMAT_BINARY_NAME_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `clang-format` pre-build binary name")
  endif()
  set(CLANG_FORMAT_BINARY_URL "https://files.pythonhosted.org/packages/${${CLANG_FORMAT_BINARY_CHECKSUM_VAR}}/${${CLANG_FORMAT_BINARY_URL_VAR}}")

  # Download and extract the pre-built binary ZIP if needed
  set(CLANG_FORMAT_BINARY_NAME "${${CLANG_FORMAT_BINARY_NAME_VAR}}")
  set(CLANG_FORMAT_BINARY_OUTPUT "${SOURCEMETA_TARGET_CLANG_FORMAT_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}/${CLANG_FORMAT_BINARY_NAME}")
  if(EXISTS "${CLANG_FORMAT_BINARY_OUTPUT}")
    message(STATUS "Found existing `clang-format` pre-built binary at ${CLANG_FORMAT_BINARY_OUTPUT}")
    return()
  endif()
  set(CLANG_FORMAT_BINARY_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/clang-format")
  file(REMOVE_RECURSE "${CLANG_FORMAT_BINARY_DOWNLOAD_DIR}")
  file(MAKE_DIRECTORY "${CLANG_FORMAT_BINARY_DOWNLOAD_DIR}")
  set(CLANG_FORMAT_BINARY_WHEEL "${CLANG_FORMAT_BINARY_DOWNLOAD_DIR}/clang-format.whl")
  message(STATUS "Downloading `clang-format` pre-built binary from ${CLANG_FORMAT_BINARY_URL}")
  file(DOWNLOAD "${CLANG_FORMAT_BINARY_URL}" "${CLANG_FORMAT_BINARY_WHEEL}"
    STATUS CLANG_FORMAT_BINARY_DOWNLOAD_STATUS SHOW_PROGRESS TLS_VERIFY ON
    LOG CLANG_FORMAT_BINARY_DOWNLOAD_LOG)
  list(GET CLANG_FORMAT_BINARY_DOWNLOAD_STATUS 0 _code)
  if(NOT _code EQUAL 0)
    message(WARNING "Failed to download the `clang-format` pre-built binary")
    message(WARNING "${CLANG_FORMAT_BINARY_DOWNLOAD_LOG}")
    file(REMOVE_RECURSE "${CLANG_FORMAT_BINARY_DOWNLOAD_DIR}")
    return()
  endif()
  set(CLANG_FORMAT_BINARY_EXTRACT_DIR "${CLANG_FORMAT_BINARY_DOWNLOAD_DIR}/extracted")
  file(MAKE_DIRECTORY "${CLANG_FORMAT_BINARY_EXTRACT_DIR}")
  file(ARCHIVE_EXTRACT INPUT "${CLANG_FORMAT_BINARY_WHEEL}" DESTINATION "${CLANG_FORMAT_BINARY_EXTRACT_DIR}")

  # Install the binary
  file(MAKE_DIRECTORY "${SOURCEMETA_TARGET_CLANG_FORMAT_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(COPY "${CLANG_FORMAT_BINARY_EXTRACT_DIR}/clang_format/data/bin/${CLANG_FORMAT_BINARY_NAME}"
       DESTINATION "${SOURCEMETA_TARGET_CLANG_FORMAT_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(CHMOD "${CLANG_FORMAT_BINARY_OUTPUT}" PERMISSIONS
       OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  message(STATUS "Installed `clang-format` pre-built binary to ${CLANG_FORMAT_BINARY_OUTPUT}")
endfunction()

function(sourcemeta_target_clang_format)
  cmake_parse_arguments(SOURCEMETA_TARGET_CLANG_FORMAT "REQUIRED" "" "SOURCES" ${ARGN})
  sourcemeta_target_clang_format_attempt_install(OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

  if(SOURCEMETA_TARGET_CLANG_FORMAT_REQUIRED)
    find_program(CLANG_FORMAT_BIN NAMES clang-format NO_DEFAULT_PATH
      PATHS "${CMAKE_CURRENT_BINARY_DIR}/bin")
    if(NOT CLANG_FORMAT_BIN)
      find_program(CLANG_FORMAT_BIN NAMES clang-format REQUIRED)
    endif()
  else()
    find_program(CLANG_FORMAT_BIN NAMES clang-format NO_DEFAULT_PATH
      PATHS "${CMAKE_CURRENT_BINARY_DIR}/bin")
    if(NOT CLANG_FORMAT_BIN)
      find_program(CLANG_FORMAT_BIN NAMES clang-format)
    endif()
  endif()

  # This covers the empty list too
  if(NOT SOURCEMETA_TARGET_CLANG_FORMAT_SOURCES)
    message(FATAL_ERROR "You must pass file globs to format in the SOURCES option")
  endif()
  file(GLOB_RECURSE SOURCEMETA_TARGET_CLANG_FORMAT_FILES
    ${SOURCEMETA_TARGET_CLANG_FORMAT_SOURCES})

  set(CLANG_FORMAT_CONFIG "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/clang-format.json")
  if(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    # Because `clang-format` is typically a Windows `.exe`, transform the path accordingly
    execute_process(COMMAND cygpath -w "${CLANG_FORMAT_CONFIG}"
      OUTPUT_VARIABLE CLANG_FORMAT_CONFIG OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  if(CLANG_FORMAT_BIN)
    message(STATUS "Using `clang-format` from ${CLANG_FORMAT_BIN}")
    add_custom_target(clang_format
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CLANG_FORMAT_BIN}" "--style=file:${CLANG_FORMAT_CONFIG}"
        -i ${SOURCEMETA_TARGET_CLANG_FORMAT_FILES}
      COMMENT "Formatting sources using ClangFormat")
    add_custom_target(clang_format_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CLANG_FORMAT_BIN}" "--style=file:${CLANG_FORMAT_CONFIG}"
        --dry-run -Werror
        -i ${SOURCEMETA_TARGET_CLANG_FORMAT_FILES}
      COMMENT "Checking for ClangFormat compliance")
  else()
    add_custom_target(clang_format
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate ClangFormat"
      COMMAND "${CMAKE_COMMAND}" -E false)
    add_custom_target(clang_format_test
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate ClangFormat"
      COMMAND "${CMAKE_COMMAND}" -E false)
  endif()

  set_target_properties(clang_format clang_format_test PROPERTIES FOLDER "Formatting")
endfunction()
