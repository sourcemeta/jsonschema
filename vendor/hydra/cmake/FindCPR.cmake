if(NOT CPR_FOUND)
  set(CPR_DIR "${PROJECT_SOURCE_DIR}/vendor/cpr")

  set(CPR_PUBLIC_HEADER "${CPR_DIR}/include/cpr/cpr.h")
  set(CPR_PRIVATE_HEADERS
    "${CPR_DIR}/include/cpr/accept_encoding.h"
    "${CPR_DIR}/include/cpr/api.h"
    "${CPR_DIR}/include/cpr/async_wrapper.h"
    "${CPR_DIR}/include/cpr/async.h"
    "${CPR_DIR}/include/cpr/auth.h"
    "${CPR_DIR}/include/cpr/bearer.h"
    "${CPR_DIR}/include/cpr/body_view.h"
    "${CPR_DIR}/include/cpr/body.h"
    "${CPR_DIR}/include/cpr/buffer.h"
    "${CPR_DIR}/include/cpr/callback.h"
    "${CPR_DIR}/include/cpr/cert_info.h"
    "${CPR_DIR}/include/cpr/connect_timeout.h"
    "${CPR_DIR}/include/cpr/cookies.h"
    "${CPR_DIR}/include/cpr/cprtypes.h"
    "${CPR_DIR}/include/cpr/curl_container.h"
    "${CPR_DIR}/include/cpr/curlholder.h"
    "${CPR_DIR}/include/cpr/curlmultiholder.h"
    "${CPR_DIR}/include/cpr/error.h"
    "${CPR_DIR}/include/cpr/file.h"
    "${CPR_DIR}/include/cpr/filesystem.h"
    "${CPR_DIR}/include/cpr/http_version.h"
    "${CPR_DIR}/include/cpr/interceptor.h"
    "${CPR_DIR}/include/cpr/interface.h"
    "${CPR_DIR}/include/cpr/limit_rate.h"
    "${CPR_DIR}/include/cpr/local_port_range.h"
    "${CPR_DIR}/include/cpr/local_port.h"
    "${CPR_DIR}/include/cpr/low_speed.h"
    "${CPR_DIR}/include/cpr/multipart.h"
    "${CPR_DIR}/include/cpr/multiperform.h"
    "${CPR_DIR}/include/cpr/parameters.h"
    "${CPR_DIR}/include/cpr/payload.h"
    "${CPR_DIR}/include/cpr/proxies.h"
    "${CPR_DIR}/include/cpr/proxyauth.h"
    "${CPR_DIR}/include/cpr/range.h"
    "${CPR_DIR}/include/cpr/redirect.h"
    "${CPR_DIR}/include/cpr/reserve_size.h"
    "${CPR_DIR}/include/cpr/resolve.h"
    "${CPR_DIR}/include/cpr/response.h"
    "${CPR_DIR}/include/cpr/secure_string.h"
    "${CPR_DIR}/include/cpr/session.h"
    "${CPR_DIR}/include/cpr/singleton.h"
    "${CPR_DIR}/include/cpr/ssl_ctx.h"
    "${CPR_DIR}/include/cpr/ssl_options.h"
    "${CPR_DIR}/include/cpr/status_codes.h"
    "${CPR_DIR}/include/cpr/threadpool.h"
    "${CPR_DIR}/include/cpr/timeout.h"
    "${CPR_DIR}/include/cpr/unix_socket.h"
    "${CPR_DIR}/include/cpr/user_agent.h"
    "${CPR_DIR}/include/cpr/util.h"
    "${CPR_DIR}/include/cpr/verbose.h")

  add_library(cpr
    "${CPR_PUBLIC_HEADER}" ${CPR_PRIVATE_HEADERS}

    "${CPR_DIR}/cpr/accept_encoding.cpp"
    "${CPR_DIR}/cpr/async.cpp"
    "${CPR_DIR}/cpr/auth.cpp"
    "${CPR_DIR}/cpr/callback.cpp"
    "${CPR_DIR}/cpr/cert_info.cpp"
    "${CPR_DIR}/cpr/cookies.cpp"
    "${CPR_DIR}/cpr/cprtypes.cpp"
    "${CPR_DIR}/cpr/curl_container.cpp"
    "${CPR_DIR}/cpr/curlholder.cpp"
    "${CPR_DIR}/cpr/curlmultiholder.cpp"
    "${CPR_DIR}/cpr/error.cpp"
    "${CPR_DIR}/cpr/file.cpp"
    "${CPR_DIR}/cpr/interceptor.cpp"
    "${CPR_DIR}/cpr/multipart.cpp"
    "${CPR_DIR}/cpr/multiperform.cpp"
    "${CPR_DIR}/cpr/parameters.cpp"
    "${CPR_DIR}/cpr/payload.cpp"
    "${CPR_DIR}/cpr/proxies.cpp"
    "${CPR_DIR}/cpr/proxyauth.cpp"
    "${CPR_DIR}/cpr/redirect.cpp"
    "${CPR_DIR}/cpr/response.cpp"
    "${CPR_DIR}/cpr/session.cpp"
    "${CPR_DIR}/cpr/ssl_ctx.cpp"
    "${CPR_DIR}/cpr/threadpool.cpp"
    "${CPR_DIR}/cpr/timeout.cpp"
    "${CPR_DIR}/cpr/unix_socket.cpp"
    "${CPR_DIR}/cpr/util.cpp")

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(cpr PRIVATE /W3 /MP /EHsc /wd4996 /wd4244 /GS-)
    target_compile_definitions(cpr PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(cpr PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wdouble-promotion
      -Wfloat-equal
      -Wformat=2
      -Wmissing-declarations
      -Wnon-virtual-dtor
      -Woverloaded-virtual
      -Wshadow
      -Wwrite-strings
      -Wno-cast-align
      -Wno-cast-qual
      -Wno-newline-eof
      -Wno-implicit-int-conversion
      -Wno-conversion
      -Wno-strict-overflow)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(cpr PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()
  endif()

  # We don't need to know the actual version
  set(cpr_VERSION "0.0.0")
  set(cpr_VERSION_MAJOR 0)
  set(cpr_VERSION_MINOR 0)
  set(cpr_VERSION_PATCH 0)
  set(cpr_VERSION_NUM 0x000000)
  configure_file("${CPR_DIR}/cmake/cprver.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cpr_generated/cpr/cprver.h")

  target_include_directories(cpr PUBLIC
    "$<BUILD_INTERFACE:${CPR_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cpr_generated>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_link_libraries(cpr PUBLIC CURL::libcurl)

  add_library(CPR::cpr ALIAS cpr)

  set_target_properties(cpr
    PROPERTIES
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME cpr
      CXX_VISIBILITY_PRESET "default"
      CXX_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME cpr)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS cpr
      EXPORT cpr
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT cpr
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cpr"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cpr-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/cpr.cmake\")\n"
      "check_required_components(\"cpr\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cpr-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cpr"
      COMPONENT sourcemeta_hydra_dev)

    install(DIRECTORY "${CPR_DIR}/include/cpr"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cpr_generated/cpr/cprver.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/cpr"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(CPR_FOUND ON)
endif()
