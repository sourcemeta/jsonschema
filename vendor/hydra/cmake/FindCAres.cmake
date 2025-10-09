if(NOT CAres_FOUND)
  set(CARES_DIR "${PROJECT_SOURCE_DIR}/vendor/c-ares")

  file(STRINGS "${PROJECT_SOURCE_DIR}/DEPENDENCIES" CARES_VERSION_LINE
    REGEX "^c-ares ")
  if(CARES_VERSION_LINE)
    string(REGEX MATCH "v?([0-9]+)\\.([0-9]+)\\.([0-9]+)" CARES_VERSION_MATCH "${CARES_VERSION_LINE}")
    set(CARES_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(CARES_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(CARES_VERSION_PATCH "${CMAKE_MATCH_3}")
    set(CARES_VERSION_STRING "${CARES_VERSION_MAJOR}.${CARES_VERSION_MINOR}.${CARES_VERSION_PATCH}")
  else()
    message(FATAL_ERROR "Could not find c-ares version in DEPENDENCIES")
  endif()

  set(CARES_PUBLIC_HEADERS
    "${CARES_DIR}/include/ares.h"
    "${CARES_DIR}/include/ares_nameser.h"
    "${CARES_DIR}/include/ares_dns.h"
    "${CARES_DIR}/include/ares_dns_record.h"
    "${CARES_DIR}/include/ares_version.h")

  set(CARES_SOURCES
    "${CARES_DIR}/src/lib/ares_addrinfo_localhost.c"
    "${CARES_DIR}/src/lib/ares_addrinfo2hostent.c"
    "${CARES_DIR}/src/lib/ares_android.c"
    "${CARES_DIR}/src/lib/ares_cancel.c"
    "${CARES_DIR}/src/lib/ares_close_sockets.c"
    "${CARES_DIR}/src/lib/ares_conn.c"
    "${CARES_DIR}/src/lib/ares_cookie.c"
    "${CARES_DIR}/src/lib/ares_data.c"
    "${CARES_DIR}/src/lib/ares_destroy.c"
    "${CARES_DIR}/src/lib/ares_free_hostent.c"
    "${CARES_DIR}/src/lib/ares_free_string.c"
    "${CARES_DIR}/src/lib/ares_freeaddrinfo.c"
    "${CARES_DIR}/src/lib/ares_getaddrinfo.c"
    "${CARES_DIR}/src/lib/ares_getenv.c"
    "${CARES_DIR}/src/lib/ares_gethostbyaddr.c"
    "${CARES_DIR}/src/lib/ares_gethostbyname.c"
    "${CARES_DIR}/src/lib/ares_getnameinfo.c"
    "${CARES_DIR}/src/lib/ares_hosts_file.c"
    "${CARES_DIR}/src/lib/ares_init.c"
    "${CARES_DIR}/src/lib/ares_library_init.c"
    "${CARES_DIR}/src/lib/ares_metrics.c"
    "${CARES_DIR}/src/lib/ares_options.c"
    "${CARES_DIR}/src/lib/ares_parse_into_addrinfo.c"
    "${CARES_DIR}/src/lib/ares_process.c"
    "${CARES_DIR}/src/lib/ares_qcache.c"
    "${CARES_DIR}/src/lib/ares_query.c"
    "${CARES_DIR}/src/lib/ares_search.c"
    "${CARES_DIR}/src/lib/ares_send.c"
    "${CARES_DIR}/src/lib/ares_set_socket_functions.c"
    "${CARES_DIR}/src/lib/ares_socket.c"
    "${CARES_DIR}/src/lib/ares_sortaddrinfo.c"
    "${CARES_DIR}/src/lib/ares_strerror.c"
    "${CARES_DIR}/src/lib/ares_sysconfig_files.c"
    "${CARES_DIR}/src/lib/ares_sysconfig_mac.c"
    "${CARES_DIR}/src/lib/ares_sysconfig_win.c"
    "${CARES_DIR}/src/lib/ares_sysconfig.c"
    "${CARES_DIR}/src/lib/ares_timeout.c"
    "${CARES_DIR}/src/lib/ares_update_servers.c"
    "${CARES_DIR}/src/lib/ares_version.c"
    "${CARES_DIR}/src/lib/inet_net_pton.c"
    "${CARES_DIR}/src/lib/inet_ntop.c"
    "${CARES_DIR}/src/lib/windows_port.c"
    "${CARES_DIR}/src/lib/dsa/ares_array.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_asvp.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_dict.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_strvp.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_szvp.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_vpstr.c"
    "${CARES_DIR}/src/lib/dsa/ares_htable_vpvp.c"
    "${CARES_DIR}/src/lib/dsa/ares_llist.c"
    "${CARES_DIR}/src/lib/dsa/ares_slist.c"
    "${CARES_DIR}/src/lib/event/ares_event_configchg.c"
    "${CARES_DIR}/src/lib/event/ares_event_epoll.c"
    "${CARES_DIR}/src/lib/event/ares_event_kqueue.c"
    "${CARES_DIR}/src/lib/event/ares_event_poll.c"
    "${CARES_DIR}/src/lib/event/ares_event_select.c"
    "${CARES_DIR}/src/lib/event/ares_event_thread.c"
    "${CARES_DIR}/src/lib/event/ares_event_wake_pipe.c"
    "${CARES_DIR}/src/lib/event/ares_event_win32.c"
    "${CARES_DIR}/src/lib/record/ares_dns_mapping.c"
    "${CARES_DIR}/src/lib/record/ares_dns_multistring.c"
    "${CARES_DIR}/src/lib/record/ares_dns_name.c"
    "${CARES_DIR}/src/lib/record/ares_dns_parse.c"
    "${CARES_DIR}/src/lib/record/ares_dns_record.c"
    "${CARES_DIR}/src/lib/record/ares_dns_write.c"
    "${CARES_DIR}/src/lib/str/ares_buf.c"
    "${CARES_DIR}/src/lib/str/ares_str.c"
    "${CARES_DIR}/src/lib/str/ares_strsplit.c"
    "${CARES_DIR}/src/lib/util/ares_iface_ips.c"
    "${CARES_DIR}/src/lib/util/ares_math.c"
    "${CARES_DIR}/src/lib/util/ares_rand.c"
    "${CARES_DIR}/src/lib/util/ares_threads.c"
    "${CARES_DIR}/src/lib/util/ares_timeval.c"
    "${CARES_DIR}/src/lib/util/ares_uri.c"
    "${CARES_DIR}/src/lib/legacy/ares_create_query.c"
    "${CARES_DIR}/src/lib/legacy/ares_expand_name.c"
    "${CARES_DIR}/src/lib/legacy/ares_expand_string.c"
    "${CARES_DIR}/src/lib/legacy/ares_fds.c"
    "${CARES_DIR}/src/lib/legacy/ares_getsock.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_a_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_aaaa_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_caa_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_mx_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_naptr_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_ns_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_ptr_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_soa_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_srv_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_txt_reply.c"
    "${CARES_DIR}/src/lib/legacy/ares_parse_uri_reply.c")

  add_library(cares ${CARES_SOURCES})

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(cares PRIVATE /W3 /wd4996 /wd4013 /MP)
    set(CARES_TYPEOF_ARES_SOCKLEN_T "int")
    set(CARES_TYPEOF_ARES_SSIZE_T "SSIZE_T")
    set(RECV_TYPE_ARG1 "SOCKET")
    set(RECV_TYPE_ARG2 "char *")
    set(RECV_TYPE_ARG3 "int")
    set(RECV_TYPE_ARG4 "int")
    set(RECVFROM_TYPE_ARG1 "SOCKET")
    set(RECVFROM_TYPE_ARG2 "char *")
    set(RECVFROM_TYPE_ARG3 "int")
    set(RECVFROM_TYPE_ARG4 "int")
    set(RECVFROM_TYPE_ARG5 "struct sockaddr *")
    set(RECVFROM_TYPE_ARG6 "int *")
    set(SEND_TYPE_ARG1 "SOCKET")
    set(SEND_TYPE_ARG2 "const char *")
    set(SEND_TYPE_ARG3 "int")
    set(SEND_TYPE_ARG4 "int")
    set(GETHOSTNAME_TYPE_ARG2 "int")
    set(GETNAMEINFO_TYPE_ARG1 "struct sockaddr *")
    set(GETNAMEINFO_TYPE_ARG2 "int")
    set(GETNAMEINFO_TYPE_ARG46 "DWORD")
    set(GETNAMEINFO_TYPE_ARG7 "int")
    target_compile_definitions(cares PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(cares PRIVATE HAVE_WINDOWS_H)
    target_compile_definitions(cares PRIVATE HAVE_WINSOCK2_H)
    target_compile_definitions(cares PRIVATE HAVE_WS2TCPIP_H)
    target_compile_definitions(cares PRIVATE HAVE_IPHLPAPI_H)
    target_compile_definitions(cares PRIVATE HAVE_NETIOAPI_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_STAT_H)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_TIMEVAL)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_SOCKADDR_IN6)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_ADDRINFO)
    target_compile_definitions(cares PRIVATE HAVE_AF_INET6)
    target_compile_definitions(cares PRIVATE HAVE_RECV)
    target_compile_definitions(cares PRIVATE HAVE_RECVFROM)
    target_compile_definitions(cares PRIVATE HAVE_SEND)
    target_compile_definitions(cares PRIVATE HAVE_SENDTO)
    target_compile_definitions(cares PRIVATE HAVE_IOCTLSOCKET)
    target_compile_definitions(cares PRIVATE HAVE_IOCTLSOCKET_FIONBIO)
    target_compile_definitions(cares PRIVATE HAVE_ERRNO_H)
    target_compile_definitions(cares PRIVATE HAVE_STDLIB_H)
    target_compile_definitions(cares PRIVATE HAVE_STRING_H)
    target_compile_definitions(cares PRIVATE HAVE_LIMITS_H)
    target_compile_definitions(cares PRIVATE HAVE_STDINT_H)
    target_compile_definitions(cares PRIVATE _WIN32_WINNT=0x0600)
    target_link_libraries(cares PRIVATE ws2_32)
    target_link_libraries(cares PRIVATE iphlpapi)
  else()
    target_compile_options(cares PRIVATE
      -Wall
      -Wextra
      -Wno-unused-parameter
      -Wno-sign-conversion
      -Wno-sign-compare
      -Wno-shadow
      -Wno-unused-variable
      -Wno-unused-function)

    if(HYDRA_COMPILER_GCC)
      target_compile_options(cares PRIVATE -Wno-stringop-overflow -Wno-discarded-qualifiers)
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(cares PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()

    set(CARES_TYPEOF_ARES_SOCKLEN_T "socklen_t")
    set(CARES_TYPEOF_ARES_SSIZE_T "ssize_t")
    set(RECV_TYPE_ARG1 "int")
    set(RECV_TYPE_ARG2 "void *")
    set(RECV_TYPE_ARG3 "size_t")
    set(RECV_TYPE_ARG4 "int")
    set(RECVFROM_TYPE_ARG1 "int")
    set(RECVFROM_TYPE_ARG2 "void *")
    set(RECVFROM_TYPE_ARG3 "size_t")
    set(RECVFROM_TYPE_ARG4 "int")
    set(RECVFROM_TYPE_ARG5 "struct sockaddr *")
    set(RECVFROM_TYPE_ARG6 "socklen_t *")
    set(SEND_TYPE_ARG1 "int")
    set(SEND_TYPE_ARG2 "const void *")
    set(SEND_TYPE_ARG3 "size_t")
    set(SEND_TYPE_ARG4 "int")
    set(GETHOSTNAME_TYPE_ARG2 "size_t")
    set(GETNAMEINFO_TYPE_ARG1 "struct sockaddr *")
    set(GETNAMEINFO_TYPE_ARG2 "socklen_t")
    set(GETNAMEINFO_TYPE_ARG46 "size_t")
    set(GETNAMEINFO_TYPE_ARG7 "int")

    if(NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "MSYS")
      target_compile_definitions(cares PRIVATE _POSIX_C_SOURCE=199309L)
    endif()

    target_compile_definitions(cares PRIVATE HAVE_SYS_TIME_H)
    target_compile_definitions(cares PRIVATE HAVE_TIME_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_SELECT_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_TYPES_H)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_TIMEVAL)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_SOCKADDR_IN6)
    target_compile_definitions(cares PRIVATE HAVE_STRUCT_ADDRINFO)
    target_compile_definitions(cares PRIVATE HAVE_AF_INET6)
    target_compile_definitions(cares PRIVATE HAVE_PF_INET6)
    target_compile_definitions(cares PRIVATE HAVE_NETDB_H)
    target_compile_definitions(cares PRIVATE HAVE_ARPA_INET_H)
    target_compile_definitions(cares PRIVATE HAVE_NETINET_IN_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_SOCKET_H)
    target_compile_definitions(cares PRIVATE HAVE_UNISTD_H)
    target_compile_definitions(cares PRIVATE HAVE_FCNTL_H)
    target_compile_definitions(cares PRIVATE HAVE_ERRNO_H)
    target_compile_definitions(cares PRIVATE HAVE_STDLIB_H)
    target_compile_definitions(cares PRIVATE HAVE_STRING_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_IOCTL_H)
    target_compile_definitions(cares PRIVATE HAVE_PTHREAD_H)
    target_compile_definitions(cares PRIVATE HAVE_LIMITS_H)
    target_compile_definitions(cares PRIVATE HAVE_SYS_UIO_H)
    target_compile_definitions(cares PRIVATE HAVE_FCNTL_O_NONBLOCK)
    target_compile_definitions(cares PRIVATE HAVE_RECV)
    target_compile_definitions(cares PRIVATE HAVE_RECVFROM)
    target_compile_definitions(cares PRIVATE HAVE_SEND)
    target_compile_definitions(cares PRIVATE HAVE_SENDTO)
    target_compile_definitions(cares PRIVATE HAVE_WRITEV)
    target_compile_definitions(cares PRIVATE HAVE_GETTIMEOFDAY)
    target_compile_definitions(cares PRIVATE HAVE_CLOCK_GETTIME_MONOTONIC)
    target_compile_definitions(cares PRIVATE HAVE_STDINT_H)

    if(APPLE)
      target_link_libraries(cares PRIVATE "-framework CoreFoundation")
      target_link_libraries(cares PRIVATE "-framework SystemConfiguration")
    endif()
  endif()

  configure_file("${CARES_DIR}/include/ares_build.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cares_generated/ares_build.h" @ONLY)
  configure_file("${CARES_DIR}/src/lib/ares_config.h.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cares_generated/ares_config.h" @ONLY)

  target_compile_definitions(cares PRIVATE HAVE_CONFIG_H)
  target_compile_definitions(cares PRIVATE CARES_BUILDING_LIBRARY)
  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(cares PUBLIC CARES_STATICLIB)
  endif()

  target_include_directories(cares PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/cares_generated")
  target_include_directories(cares PRIVATE "${CARES_DIR}/src/lib")
  target_include_directories(cares PRIVATE "${CARES_DIR}/src/lib/include")
  target_include_directories(cares PUBLIC
    "$<BUILD_INTERFACE:${CARES_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/cares_generated>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(c-ares::cares ALIAS cares)

  set_target_properties(cares
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME cares
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      WINDOWS_EXPORT_ALL_SYMBOLS TRUE
      EXPORT_NAME cares)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS cares
      EXPORT cares
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT cares
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cares"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cares-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/cares.cmake\")\n"
      "check_required_components(\"cares\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/cares-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cares"
      COMPONENT sourcemeta_hydra_dev)

    install(FILES ${CARES_PUBLIC_HEADERS}
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cares_generated/ares_build.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(CAres_FOUND ON)
endif()
