cmake_minimum_required(VERSION 3.16)
file(READ "${CMAKE_CURRENT_LIST_DIR}/../VERSION" JSONSCHEMA_VERSION)
string(STRIP "${JSONSCHEMA_VERSION}" JSONSCHEMA_VERSION)

set(BASE_URL
  "https://github.com/sourcemeta/jsonschema/releases/download/v${JSONSCHEMA_VERSION}")
set(RELEASES_DIR "${CMAKE_CURRENT_LIST_DIR}/../build/github-releases")
file(MAKE_DIRECTORY "${RELEASES_DIR}")

foreach(PLATFORM
  darwin-arm64
  darwin-x86_64
  linux-x86_64
  linux-arm64
  linux-x86_64-musl
  windows-x86_64)
  if(PLATFORM MATCHES "windows")
    set(OUTPUT_BINARY "${RELEASES_DIR}/jsonschema-${PLATFORM}.exe")
    set(INNER_BINARY "bin/jsonschema.exe")
  else()
    set(OUTPUT_BINARY "${RELEASES_DIR}/jsonschema-${PLATFORM}")
    set(INNER_BINARY "bin/jsonschema")
  endif()

  if(EXISTS "${OUTPUT_BINARY}")
    file(SHA256 "${OUTPUT_BINARY}" EXISTING_HASH)
    if(NOT EXISTING_HASH STREQUAL "")
      message(STATUS "Cached: ${OUTPUT_BINARY}")
      continue()
    endif()
  endif()

  set(ZIP_URL "${BASE_URL}/jsonschema-${JSONSCHEMA_VERSION}-${PLATFORM}.zip")
  set(ZIP_PATH "${RELEASES_DIR}/${PLATFORM}.zip")
  message(STATUS "Downloading ${ZIP_URL}")
  file(DOWNLOAD "${ZIP_URL}" "${ZIP_PATH}" STATUS DOWNLOAD_STATUS SHOW_PROGRESS)
  list(GET DOWNLOAD_STATUS 0 ERROR_CODE)
  if(NOT ERROR_CODE EQUAL 0)
    list(GET DOWNLOAD_STATUS 1 ERROR_MESSAGE)
    message(FATAL_ERROR "Download failed: ${ERROR_MESSAGE}")
  endif()

  file(ARCHIVE_EXTRACT INPUT "${ZIP_PATH}" DESTINATION "${RELEASES_DIR}")
  file(RENAME
    "${RELEASES_DIR}/jsonschema-${JSONSCHEMA_VERSION}-${PLATFORM}/${INNER_BINARY}"
    "${OUTPUT_BINARY}")
  file(REMOVE_RECURSE
    "${RELEASES_DIR}/jsonschema-${JSONSCHEMA_VERSION}-${PLATFORM}")
  file(REMOVE "${ZIP_PATH}")
  if(NOT PLATFORM MATCHES "windows")
    file(CHMOD "${OUTPUT_BINARY}" PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_EXECUTE
      WORLD_READ WORLD_EXECUTE)
  endif()
endforeach()

message(STATUS "Binaries in: ${RELEASES_DIR}")
