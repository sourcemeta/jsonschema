if(NOT uWebSockets_FOUND)
  set(UWEBSOCKETS_DIR "${PROJECT_SOURCE_DIR}/vendor/uwebsockets")
  set(UWEBSOCKETS_PUBLIC_HEADER "${UWEBSOCKETS_DIR}/src/App.h")
  set(UWEBSOCKETS_PRIVATE_HEADERS
    "${UWEBSOCKETS_DIR}/src/LocalCluster.h"
    "${UWEBSOCKETS_DIR}/src/AsyncSocket.h"
    "${UWEBSOCKETS_DIR}/src/AsyncSocketData.h"
    "${UWEBSOCKETS_DIR}/src/BloomFilter.h"
    "${UWEBSOCKETS_DIR}/src/ChunkedEncoding.h"
    "${UWEBSOCKETS_DIR}/src/ClientApp.h"
    "${UWEBSOCKETS_DIR}/src/Http3App.h"
    "${UWEBSOCKETS_DIR}/src/Http3Context.h"
    "${UWEBSOCKETS_DIR}/src/Http3ContextData.h"
    "${UWEBSOCKETS_DIR}/src/Http3Request.h"
    "${UWEBSOCKETS_DIR}/src/Http3Response.h"
    "${UWEBSOCKETS_DIR}/src/Http3ResponseData.h"
    "${UWEBSOCKETS_DIR}/src/HttpContext.h"
    "${UWEBSOCKETS_DIR}/src/HttpContextData.h"
    "${UWEBSOCKETS_DIR}/src/HttpErrors.h"
    "${UWEBSOCKETS_DIR}/src/HttpParser.h"
    "${UWEBSOCKETS_DIR}/src/HttpResponse.h"
    "${UWEBSOCKETS_DIR}/src/HttpResponseData.h"
    "${UWEBSOCKETS_DIR}/src/HttpRouter.h"
    "${UWEBSOCKETS_DIR}/src/Loop.h"
    "${UWEBSOCKETS_DIR}/src/LoopData.h"
    "${UWEBSOCKETS_DIR}/src/MessageParser.h"
    "${UWEBSOCKETS_DIR}/src/MoveOnlyFunction.h"
    "${UWEBSOCKETS_DIR}/src/Multipart.h"
    "${UWEBSOCKETS_DIR}/src/PerMessageDeflate.h"
    "${UWEBSOCKETS_DIR}/src/ProxyParser.h"
    "${UWEBSOCKETS_DIR}/src/QueryParser.h"
    "${UWEBSOCKETS_DIR}/src/TopicTree.h"
    "${UWEBSOCKETS_DIR}/src/Utilities.h"
    "${UWEBSOCKETS_DIR}/src/WebSocket.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketContext.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketContextData.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketData.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketExtensions.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketHandshake.h"
    "${UWEBSOCKETS_DIR}/src/WebSocketProtocol.h")

  add_library(uwebsockets INTERFACE
    "${UWEBSOCKETS_PUBLIC_HEADER}" ${UWEBSOCKETS_PRIVATE_HEADERS})

  # Avoid "uWebSockets: <version>" default header
  target_compile_definitions(uwebsockets
    INTERFACE UWS_HTTPRESPONSE_NO_WRITEMARK)

  target_link_libraries(uwebsockets INTERFACE ZLIB::ZLIB)
  target_link_libraries(uwebsockets INTERFACE uNetworking::uSockets)

  target_include_directories(usockets PUBLIC
    "$<BUILD_INTERFACE:${UWEBSOCKETS_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/uwebsockets>")

  add_library(uNetworking::uWebSockets ALIAS uwebsockets)

  set_target_properties(uwebsockets
    PROPERTIES
      OUTPUT_NAME uwebsockets
      PUBLIC_HEADER "${UWEBSOCKETS_PUBLIC_HEADER}"
      PRIVATE_HEADER "${UWEBSOCKETS_PRIVATE_HEADERS}"
      EXPORT_NAME uwebsockets)

  if(HYDRA_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS uwebsockets
      EXPORT uwebsockets
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/uwebsockets/src"
        COMPONENT sourcemeta_hydra_dev
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/uwebsockets/src"
        COMPONENT sourcemeta_hydra_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT uwebsockets
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/uwebsockets"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/uwebsockets-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/uwebsockets.cmake\")\n"
      "check_required_components(\"uwebsockets\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/uwebsockets-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/uwebsockets"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(uWebSockets_FOUND ON)
endif()
