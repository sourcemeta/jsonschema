function(sourcemeta_ccache_attempt_install)
  cmake_parse_arguments(SOURCEMETA_CCACHE_ATTEMPT_INSTALL "" "OUTPUT_DIRECTORY" "" ${ARGN})
  if(NOT SOURCEMETA_CCACHE_ATTEMPT_INSTALL_OUTPUT_DIRECTORY)
    message(FATAL_ERROR "You must pass the output directory in the OUTPUT_DIRECTORY option")
  endif()

  # See https://github.com/ccache/ccache/releases
  set(CCACHE_BINARY_VERSION "4.14")
  set(CCACHE_BINARY_Windows_AMD64 "ccache-${CCACHE_BINARY_VERSION}-windows-x86_64.zip")
  set(CCACHE_BINARY_MSYS_x86_64 "ccache-${CCACHE_BINARY_VERSION}-windows-x86_64.zip")
  set(CCACHE_BINARY_Darwin_arm64 "ccache-${CCACHE_BINARY_VERSION}-darwin.tar.gz")
  set(CCACHE_BINARY_Darwin_x86_64 "ccache-${CCACHE_BINARY_VERSION}-darwin.tar.gz")
  set(CCACHE_BINARY_Linux_aarch64 "ccache-${CCACHE_BINARY_VERSION}-linux-aarch64-musl-static.tar.gz")
  set(CCACHE_BINARY_Linux_x86_64 "ccache-${CCACHE_BINARY_VERSION}-linux-x86_64-musl-static.tar.gz")
  set(CCACHE_BINARY_CHECKSUM_Windows_AMD64 "2568347a697e103ca1b073981c704ad76fb2507d066c38dba038dd73399d968f")
  set(CCACHE_BINARY_CHECKSUM_MSYS_x86_64 "2568347a697e103ca1b073981c704ad76fb2507d066c38dba038dd73399d968f")
  set(CCACHE_BINARY_CHECKSUM_Darwin_arm64 "353a81ea8680d93387102cfde288ebed381a54272cfdc18224f241add6332b39")
  set(CCACHE_BINARY_CHECKSUM_Darwin_x86_64 "353a81ea8680d93387102cfde288ebed381a54272cfdc18224f241add6332b39")
  set(CCACHE_BINARY_CHECKSUM_Linux_aarch64 "2218884d62433c416d5bc33cabba3d8bf7a410bd94dc4950e2625ddbd1eafd1d")
  set(CCACHE_BINARY_CHECKSUM_Linux_x86_64 "985f575acf84cf6d70e0f4fe86903418df9e7bce11f35faaf8d415e5ff5a9478")
  set(CCACHE_BINARY_NAME_Windows_AMD64 "ccache.exe")
  set(CCACHE_BINARY_NAME_MSYS_x86_64 "ccache.exe")
  set(CCACHE_BINARY_NAME_Darwin_arm64 "ccache")
  set(CCACHE_BINARY_NAME_Darwin_x86_64 "ccache")
  set(CCACHE_BINARY_NAME_Linux_aarch64 "ccache")
  set(CCACHE_BINARY_NAME_Linux_x86_64 "ccache")

  # Determine the pre-built binary URL
  string(REPLACE "." "_" CCACHE_BINARY_SYSTEM "${CMAKE_SYSTEM_NAME}")
  string(REPLACE "." "_" CCACHE_BINARY_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
  set(CCACHE_BINARY_ARCHIVE_VAR "CCACHE_BINARY_${CCACHE_BINARY_SYSTEM}_${CCACHE_BINARY_ARCH}")
  set(CCACHE_BINARY_CHECKSUM_VAR "CCACHE_BINARY_CHECKSUM_${CCACHE_BINARY_SYSTEM}_${CCACHE_BINARY_ARCH}")
  set(CCACHE_BINARY_NAME_VAR "CCACHE_BINARY_NAME_${CCACHE_BINARY_SYSTEM}_${CCACHE_BINARY_ARCH}")
  if(NOT DEFINED ${CCACHE_BINARY_ARCHIVE_VAR} OR "${${CCACHE_BINARY_ARCHIVE_VAR}}" STREQUAL "")
    message(WARNING "Skipping `ccache` download. No known pre-build binary URL")
    return()
  elseif(NOT DEFINED ${CCACHE_BINARY_CHECKSUM_VAR} OR "${${CCACHE_BINARY_CHECKSUM_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `ccache` pre-build binary checksum")
  elseif(NOT DEFINED ${CCACHE_BINARY_NAME_VAR} OR "${${CCACHE_BINARY_NAME_VAR}}" STREQUAL "")
    message(FATAL_ERROR "No known `ccache` pre-build binary name")
  endif()
  set(CCACHE_BINARY_ARCHIVE "${${CCACHE_BINARY_ARCHIVE_VAR}}")
  set(CCACHE_BINARY_URL
    "https://github.com/ccache/ccache/releases/download/v${CCACHE_BINARY_VERSION}/${CCACHE_BINARY_ARCHIVE}")

  # Download and extract the pre-built binary archive if needed
  set(CCACHE_BINARY_NAME "${${CCACHE_BINARY_NAME_VAR}}")
  set(CCACHE_BINARY_OUTPUT "${SOURCEMETA_CCACHE_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}/${CCACHE_BINARY_NAME}")
  if(EXISTS "${CCACHE_BINARY_OUTPUT}")
    message(STATUS "Found existing `ccache` pre-built binary at ${CCACHE_BINARY_OUTPUT}")
    return()
  endif()
  set(CCACHE_BINARY_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/ccache")
  file(REMOVE_RECURSE "${CCACHE_BINARY_DOWNLOAD_DIR}")
  file(MAKE_DIRECTORY "${CCACHE_BINARY_DOWNLOAD_DIR}")
  set(CCACHE_BINARY_DOWNLOAD "${CCACHE_BINARY_DOWNLOAD_DIR}/${CCACHE_BINARY_ARCHIVE}")
  message(STATUS "Downloading `ccache` pre-built binary from ${CCACHE_BINARY_URL}")
  file(DOWNLOAD "${CCACHE_BINARY_URL}" "${CCACHE_BINARY_DOWNLOAD}"
    EXPECTED_HASH "SHA256=${${CCACHE_BINARY_CHECKSUM_VAR}}"
    STATUS CCACHE_BINARY_DOWNLOAD_STATUS SHOW_PROGRESS TLS_VERIFY ON
    LOG CCACHE_BINARY_DOWNLOAD_LOG)
  list(GET CCACHE_BINARY_DOWNLOAD_STATUS 0 _code)
  if(NOT _code EQUAL 0)
    message(WARNING "Failed to download the `ccache` pre-built binary")
    message(WARNING "${CCACHE_BINARY_DOWNLOAD_LOG}")
    file(REMOVE_RECURSE "${CCACHE_BINARY_DOWNLOAD_DIR}")
    return()
  endif()
  set(CCACHE_BINARY_EXTRACT_DIR "${CCACHE_BINARY_DOWNLOAD_DIR}/extracted")
  file(MAKE_DIRECTORY "${CCACHE_BINARY_EXTRACT_DIR}")
  file(ARCHIVE_EXTRACT INPUT "${CCACHE_BINARY_DOWNLOAD}" DESTINATION "${CCACHE_BINARY_EXTRACT_DIR}")

  # Every release archive holds the binary inside a single directory named
  # after the archive itself, alongside documentation that we have no use for
  string(REGEX REPLACE "\\.(tar\\.gz|zip)$" "" CCACHE_BINARY_ARCHIVE_ROOT "${CCACHE_BINARY_ARCHIVE}")

  file(MAKE_DIRECTORY "${SOURCEMETA_CCACHE_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(COPY "${CCACHE_BINARY_EXTRACT_DIR}/${CCACHE_BINARY_ARCHIVE_ROOT}/${CCACHE_BINARY_NAME}"
       DESTINATION "${SOURCEMETA_CCACHE_ATTEMPT_INSTALL_OUTPUT_DIRECTORY}")
  file(CHMOD "${CCACHE_BINARY_OUTPUT}" PERMISSIONS
       OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  file(REMOVE_RECURSE "${CCACHE_BINARY_DOWNLOAD_DIR}")
  message(STATUS "Installed `ccache` pre-built binary to ${CCACHE_BINARY_OUTPUT}")
endfunction()

function(sourcemeta_ccache_attempt_enable)
  cmake_parse_arguments(SOURCEMETA_CCACHE_ATTEMPT_ENABLE "" "DIRECTORY" "" ${ARGN})
  if(NOT SOURCEMETA_CCACHE_ATTEMPT_ENABLE_DIRECTORY)
    message(FATAL_ERROR "You must pass the cache directory in the DIRECTORY option")
  endif()

  # Stay out of the way of a consumer that already brought its own launcher
  if(CMAKE_C_COMPILER_LAUNCHER OR CMAKE_CXX_COMPILER_LAUNCHER)
    return()
  endif()

  sourcemeta_ccache_attempt_install(OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  find_program(CCACHE_BIN NAMES ccache NO_DEFAULT_PATH
    PATHS "${PROJECT_BINARY_DIR}/bin")
  if(NOT CCACHE_BIN)
    find_program(CCACHE_BIN NAMES ccache)
  endif()
  if(NOT CCACHE_BIN)
    message(STATUS "Could not locate `ccache`. Compiling without a compiler cache")
    add_custom_target(ccache_stats
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate the compiler cache"
      COMMAND "${CMAKE_COMMAND}" -E false)
    add_custom_target(ccache_stats_zero
      VERBATIM
      COMMAND "${CMAKE_COMMAND}" -E echo "Could not locate the compiler cache"
      COMMAND "${CMAKE_COMMAND}" -E false)
    set_target_properties(ccache_stats ccache_stats_zero PROPERTIES FOLDER "Compiler cache")
    return()
  endif()

  message(STATUS "Using `ccache` from ${CCACHE_BIN}")

  set(CCACHE_DIRECTORY "${SOURCEMETA_CCACHE_ATTEMPT_ENABLE_DIRECTORY}")
  if(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    # Because `ccache` is a Windows `.exe` that does not understand the paths
    # this platform hands out, transform the path accordingly
    execute_process(COMMAND cygpath -w "${CCACHE_DIRECTORY}"
      OUTPUT_VARIABLE CCACHE_DIRECTORY OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()

  add_custom_target(ccache_stats
    VERBATIM
    COMMAND "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_DIRECTORY}"
      "${CCACHE_BIN}" --show-stats
    COMMENT "Reporting compiler cache statistics")
  add_custom_target(ccache_stats_zero
    VERBATIM
    COMMAND "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_DIRECTORY}"
      "${CCACHE_BIN}" --zero-stats
    COMMENT "Zeroing compiler cache statistics")
  set_target_properties(ccache_stats ccache_stats_zero PROPERTIES FOLDER "Compiler cache")

  if(CMAKE_GENERATOR MATCHES "Visual Studio")
    # This generator ignores the compiler launchers, so `ccache` takes the place
    # of the compiler under a matching name and the build system is pointed at
    # it. Batching several translation units into a single compiler invocation
    # is the default here, and nothing can be cached that way. Note this route
    # cannot carry the cache location, which such a build only reads from the
    # environment
    set(CCACHE_MASQUERADE_DIRECTORY "${PROJECT_BINARY_DIR}/ccache-cl")
    file(MAKE_DIRECTORY "${CCACHE_MASQUERADE_DIRECTORY}")
    file(COPY_FILE "${CCACHE_BIN}" "${CCACHE_MASQUERADE_DIRECTORY}/cl.exe" ONLY_IF_DIFFERENT)
    set(CMAKE_VS_GLOBALS
      "CLToolExe=cl.exe"
      "CLToolPath=${CCACHE_MASQUERADE_DIRECTORY}"
      "UseMultiToolTask=true"
      PARENT_SCOPE)
    return()
  endif()

  # Pinning the cache location here keeps it identical across platforms and
  # shells without asking anyone to export anything, which matters because
  # `ccache` expands neither a leading tilde nor the shell
  set(CCACHE_LAUNCHER "${CMAKE_COMMAND}" -E env "CCACHE_DIR=${CCACHE_DIRECTORY}" "${CCACHE_BIN}")
  foreach(CCACHE_LANGUAGE IN LISTS SOURCEMETA_LANGUAGES)
    if(NOT CCACHE_LANGUAGE STREQUAL "ASM_MASM")
      set("CMAKE_${CCACHE_LANGUAGE}_COMPILER_LAUNCHER" "${CCACHE_LAUNCHER}" PARENT_SCOPE)
    endif()
  endforeach()
endfunction()
