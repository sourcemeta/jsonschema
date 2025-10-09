if(NOT CURL_FOUND)
  set(CURL_DIR "${PROJECT_SOURCE_DIR}/vendor/curl")

  set(CURL_PUBLIC_HEADER "${CURL_DIR}/include/curl/curl.h")
  set(CURL_PRIVATE_HEADERS
    "${CURL_DIR}/include/curl/stdcheaders.h"
    "${CURL_DIR}/include/curl/header.h"
    "${CURL_DIR}/include/curl/options.h"
    "${CURL_DIR}/include/curl/mprintf.h"
    "${CURL_DIR}/include/curl/easy.h"
    "${CURL_DIR}/include/curl/websockets.h"
    "${CURL_DIR}/include/curl/curlver.h"
    "${CURL_DIR}/include/curl/system.h"
    "${CURL_DIR}/include/curl/typecheck-gcc.h"
    "${CURL_DIR}/include/curl/multi.h"
    "${CURL_DIR}/include/curl/urlapi.h")

  add_library(curl
    "${CURL_PUBLIC_HEADER}" ${CURL_PRIVATE_HEADERS}

    "${CURL_DIR}/lib/strcase.c"
    "${CURL_DIR}/lib/cf-h2-proxy.h"
    "${CURL_DIR}/lib/easyoptions.c"
    "${CURL_DIR}/lib/dict.c"
    "${CURL_DIR}/lib/llist.c"
    "${CURL_DIR}/lib/mprintf.c"
    "${CURL_DIR}/lib/pingpong.c"
    "${CURL_DIR}/lib/httpsrr.c"
    "${CURL_DIR}/lib/socks_gssapi.c"
    "${CURL_DIR}/lib/memdebug.h"
    "${CURL_DIR}/lib/psl.c"
    "${CURL_DIR}/lib/progress.h"
    "${CURL_DIR}/lib/url.c"
    "${CURL_DIR}/lib/easyif.h"
    "${CURL_DIR}/lib/curl_md4.h"
    "${CURL_DIR}/lib/uint-hash.h"
    "${CURL_DIR}/lib/http_aws_sigv4.h"
    "${CURL_DIR}/lib/curl_get_line.c"
    "${CURL_DIR}/lib/hmac.c"
    "${CURL_DIR}/lib/curl_addrinfo.h"
    "${CURL_DIR}/lib/md4.c"
    "${CURL_DIR}/lib/curl_range.c"
    "${CURL_DIR}/lib/uint-table.c"
    "${CURL_DIR}/lib/idn.c"
    "${CURL_DIR}/lib/multi_ev.c"
    "${CURL_DIR}/lib/smtp.h"
    "${CURL_DIR}/lib/curl_threads.c"
    "${CURL_DIR}/lib/if2ip.c"
    "${CURL_DIR}/lib/cf-socket.c"
    "${CURL_DIR}/lib/cf-ip-happy.c"
    "${CURL_DIR}/lib/uint-bset.h"
    "${CURL_DIR}/lib/http_negotiate.c"
    "${CURL_DIR}/lib/doh.c"
    "${CURL_DIR}/lib/asyn-thrdd.c"
    "${CURL_DIR}/lib/curl_endian.c"
    "${CURL_DIR}/lib/formdata.c"
    "${CURL_DIR}/lib/easygetopt.c"
    "${CURL_DIR}/lib/cf-https-connect.c"
    "${CURL_DIR}/lib/mime.h"
    "${CURL_DIR}/lib/strdup.h"
    "${CURL_DIR}/lib/fileinfo.h"
    "${CURL_DIR}/lib/curl_ldap.h"
    "${CURL_DIR}/lib/amigaos.h"
    "${CURL_DIR}/lib/telnet.h"
    "${CURL_DIR}/lib/rand.c"
    "${CURL_DIR}/lib/http_chunks.h"
    "${CURL_DIR}/lib/cshutdn.c"
    "${CURL_DIR}/lib/http2.c"
    "${CURL_DIR}/lib/strerror.h"
    "${CURL_DIR}/lib/socketpair.h"
    "${CURL_DIR}/lib/netrc.h"
    "${CURL_DIR}/lib/bufref.h"
    "${CURL_DIR}/lib/request.c"
    "${CURL_DIR}/lib/dynhds.c"
    "${CURL_DIR}/lib/curl_sasl.h"
    "${CURL_DIR}/lib/content_encoding.c"
    "${CURL_DIR}/lib/curl_ctype.h"
    "${CURL_DIR}/lib/hostip.c"
    "${CURL_DIR}/lib/curl_sspi.h"
    "${CURL_DIR}/lib/http.h"
    "${CURL_DIR}/lib/curl_des.h"
    "${CURL_DIR}/lib/curl_gethostname.h"
    "${CURL_DIR}/lib/rtsp.h"
    "${CURL_DIR}/lib/splay.h"
    "${CURL_DIR}/lib/escape.c"
    "${CURL_DIR}/lib/easy.c"
    "${CURL_DIR}/lib/rename.c"
    "${CURL_DIR}/lib/pop3.h"
    "${CURL_DIR}/lib/share.c"
    "${CURL_DIR}/lib/slist.c"
    "${CURL_DIR}/lib/tftp.c"
    "${CURL_DIR}/lib/curl_ntlm_core.h"
    "${CURL_DIR}/lib/mqtt.c"
    "${CURL_DIR}/lib/config-plan9.h"
    "${CURL_DIR}/lib/noproxy.h"
    "${CURL_DIR}/lib/gopher.h"
    "${CURL_DIR}/lib/fopen.c"
    "${CURL_DIR}/lib/multihandle.h"
    "${CURL_DIR}/lib/socks.c"
    "${CURL_DIR}/lib/imap.h"
    "${CURL_DIR}/lib/parsedate.c"
    "${CURL_DIR}/lib/curl_trc.c"
    "${CURL_DIR}/lib/hsts.h"
    "${CURL_DIR}/lib/cf-haproxy.c"
    "${CURL_DIR}/lib/http_digest.h"
    "${CURL_DIR}/lib/curl_printf.h"
    "${CURL_DIR}/lib/file.h"
    "${CURL_DIR}/lib/cfilters.c"
    "${CURL_DIR}/lib/ftp.h"
    "${CURL_DIR}/lib/cf-h1-proxy.h"
    "${CURL_DIR}/lib/smb.h"
    "${CURL_DIR}/lib/curl_memrchr.h"
    "${CURL_DIR}/lib/conncache.h"
    "${CURL_DIR}/lib/altsvc.h"
    "${CURL_DIR}/lib/ws.h"
    "${CURL_DIR}/lib/curl_sha512_256.c"
    "${CURL_DIR}/lib/connect.h"
    "${CURL_DIR}/lib/system_win32.c"
    "${CURL_DIR}/lib/transfer.c"
    "${CURL_DIR}/lib/curl_gssapi.h"
    "${CURL_DIR}/lib/curl_rtmp.c"
    "${CURL_DIR}/lib/select.c"
    "${CURL_DIR}/lib/curl_fnmatch.h"
    "${CURL_DIR}/lib/getinfo.h"
    "${CURL_DIR}/lib/fake_addrinfo.h"
    "${CURL_DIR}/lib/hostip4.c"
    "${CURL_DIR}/lib/cw-out.h"
    "${CURL_DIR}/lib/http1.c"
    "${CURL_DIR}/lib/speedcheck.h"
    "${CURL_DIR}/lib/urlapi.c"
    "${CURL_DIR}/lib/ftplistparser.h"
    "${CURL_DIR}/lib/openldap.c"
    "${CURL_DIR}/lib/getenv.c"
    "${CURL_DIR}/lib/cw-pause.h"
    "${CURL_DIR}/lib/setopt.h"
    "${CURL_DIR}/lib/hash.c"
    "${CURL_DIR}/lib/bufq.c"
    "${CURL_DIR}/lib/http_proxy.c"
    "${CURL_DIR}/lib/http_ntlm.h"
    "${CURL_DIR}/lib/vquic/curl_quiche.h"
    "${CURL_DIR}/lib/vquic/vquic.c"
    "${CURL_DIR}/lib/vquic/curl_ngtcp2.c"
    "${CURL_DIR}/lib/vquic/curl_osslq.c"
    "${CURL_DIR}/lib/vquic/vquic-tls.c"
    "${CURL_DIR}/lib/vquic/curl_quiche.c"
    "${CURL_DIR}/lib/vquic/curl_ngtcp2.h"
    "${CURL_DIR}/lib/vquic/vquic.h"
    "${CURL_DIR}/lib/vquic/vquic-tls.h"
    "${CURL_DIR}/lib/vquic/curl_osslq.h"
    "${CURL_DIR}/lib/vquic/vquic_int.h"
    "${CURL_DIR}/lib/cookie.h"
    "${CURL_DIR}/lib/krb5.c"
    "${CURL_DIR}/lib/uint-spbset.c"
    "${CURL_DIR}/lib/macos.h"
    "${CURL_DIR}/lib/headers.h"
    "${CURL_DIR}/lib/multi.c"
    "${CURL_DIR}/lib/asyn-base.c"
    "${CURL_DIR}/lib/sendf.h"
    "${CURL_DIR}/lib/curl_memory.h"
    "${CURL_DIR}/lib/cf-https-connect.h"
    "${CURL_DIR}/lib/formdata.h"
    "${CURL_DIR}/lib/urldata.h"
    "${CURL_DIR}/lib/multiif.h"
    "${CURL_DIR}/lib/curl_endian.h"
    "${CURL_DIR}/lib/config-win32.h"
    "${CURL_DIR}/lib/strdup.c"
    "${CURL_DIR}/lib/mime.c"
    "${CURL_DIR}/lib/uint-bset.c"
    "${CURL_DIR}/lib/strequal.c"
    "${CURL_DIR}/lib/cf-socket.h"
    "${CURL_DIR}/lib/cf-ip-happy.h"
    "${CURL_DIR}/lib/if2ip.h"
    "${CURL_DIR}/lib/socks_sspi.c"
    "${CURL_DIR}/lib/curl_threads.h"
    "${CURL_DIR}/lib/smtp.c"
    "${CURL_DIR}/lib/multi_ev.h"
    "${CURL_DIR}/lib/doh.h"
    "${CURL_DIR}/lib/http_negotiate.h"
    "${CURL_DIR}/lib/curl_setup_once.h"
    "${CURL_DIR}/lib/curl_get_line.h"
    "${CURL_DIR}/lib/http_aws_sigv4.c"
    "${CURL_DIR}/lib/sockaddr.h"
    "${CURL_DIR}/lib/uint-hash.c"
    "${CURL_DIR}/lib/curl_hmac.h"
    "${CURL_DIR}/lib/uint-table.h"
    "${CURL_DIR}/lib/curl_range.h"
    "${CURL_DIR}/lib/idn.h"
    "${CURL_DIR}/lib/setup-os400.h"
    "${CURL_DIR}/lib/curl_addrinfo.c"
    "${CURL_DIR}/lib/curl_setup.h"
    "${CURL_DIR}/lib/pingpong.h"
    "${CURL_DIR}/lib/llist.h"
    "${CURL_DIR}/lib/dict.h"
    "${CURL_DIR}/lib/easyoptions.h"
    "${CURL_DIR}/lib/strcase.h"
    "${CURL_DIR}/lib/cf-h2-proxy.c"
    "${CURL_DIR}/lib/url.h"
    "${CURL_DIR}/lib/psl.h"
    "${CURL_DIR}/lib/memdebug.c"
    "${CURL_DIR}/lib/progress.c"
    "${CURL_DIR}/lib/httpsrr.h"
    "${CURL_DIR}/lib/curl_sha256.h"
    "${CURL_DIR}/lib/mqtt.h"
    "${CURL_DIR}/lib/curl_ntlm_core.c"
    "${CURL_DIR}/lib/tftp.h"
    "${CURL_DIR}/lib/slist.h"
    "${CURL_DIR}/lib/share.h"
    "${CURL_DIR}/lib/rename.h"
    "${CURL_DIR}/lib/pop3.c"
    "${CURL_DIR}/lib/arpa_telnet.h"
    "${CURL_DIR}/lib/fopen.h"
    "${CURL_DIR}/lib/noproxy.c"
    "${CURL_DIR}/lib/gopher.c"
    "${CURL_DIR}/lib/rtsp.c"
    "${CURL_DIR}/lib/curl_gethostname.c"
    "${CURL_DIR}/lib/curl_des.c"
    "${CURL_DIR}/lib/functypes.h"
    "${CURL_DIR}/lib/escape.h"
    "${CURL_DIR}/lib/setup-win32.h"
    "${CURL_DIR}/lib/splay.c"
    "${CURL_DIR}/lib/curlx/timeval.c"
    "${CURL_DIR}/lib/curlx/winapi.h"
    "${CURL_DIR}/lib/curlx/timediff.c"
    "${CURL_DIR}/lib/curlx/dynbuf.c"
    "${CURL_DIR}/lib/curlx/base64.h"
    "${CURL_DIR}/lib/curlx/version_win32.c"
    "${CURL_DIR}/lib/curlx/inet_pton.c"
    "${CURL_DIR}/lib/curlx/inet_ntop.c"
    "${CURL_DIR}/lib/curlx/wait.c"
    "${CURL_DIR}/lib/curlx/warnless.c"
    "${CURL_DIR}/lib/curlx/multibyte.c"
    "${CURL_DIR}/lib/curlx/nonblock.c"
    "${CURL_DIR}/lib/curlx/strparse.h"
    "${CURL_DIR}/lib/curlx/winapi.c"
    "${CURL_DIR}/lib/curlx/timeval.h"
    "${CURL_DIR}/lib/curlx/inet_pton.h"
    "${CURL_DIR}/lib/curlx/inet_ntop.h"
    "${CURL_DIR}/lib/curlx/wait.h"
    "${CURL_DIR}/lib/curlx/curlx.h"
    "${CURL_DIR}/lib/curlx/base64.c"
    "${CURL_DIR}/lib/curlx/version_win32.h"
    "${CURL_DIR}/lib/curlx/dynbuf.h"
    "${CURL_DIR}/lib/curlx/timediff.h"
    "${CURL_DIR}/lib/curlx/warnless.h"
    "${CURL_DIR}/lib/curlx/strparse.c"
    "${CURL_DIR}/lib/curlx/multibyte.h"
    "${CURL_DIR}/lib/curlx/nonblock.h"
    "${CURL_DIR}/lib/setup-vms.h"
    "${CURL_DIR}/lib/hostip.h"
    "${CURL_DIR}/lib/content_encoding.h"
    "${CURL_DIR}/lib/http.c"
    "${CURL_DIR}/lib/curl_sspi.c"
    "${CURL_DIR}/lib/http_chunks.c"
    "${CURL_DIR}/lib/config-os400.h"
    "${CURL_DIR}/lib/rand.h"
    "${CURL_DIR}/lib/cshutdn.h"
    "${CURL_DIR}/lib/telnet.c"
    "${CURL_DIR}/lib/amigaos.c"
    "${CURL_DIR}/lib/asyn.h"
    "${CURL_DIR}/lib/fileinfo.c"
    "${CURL_DIR}/lib/version.c"
    "${CURL_DIR}/lib/ldap.c"
    "${CURL_DIR}/lib/bufref.c"
    "${CURL_DIR}/lib/curl_sasl.c"
    "${CURL_DIR}/lib/request.h"
    "${CURL_DIR}/lib/dynhds.h"
    "${CURL_DIR}/lib/netrc.c"
    "${CURL_DIR}/lib/socketpair.c"
    "${CURL_DIR}/lib/strerror.c"
    "${CURL_DIR}/lib/http2.h"
    "${CURL_DIR}/lib/altsvc.c"
    "${CURL_DIR}/lib/config-riscos.h"
    "${CURL_DIR}/lib/conncache.c"
    "${CURL_DIR}/lib/curl_memrchr.c"
    "${CURL_DIR}/lib/smb.c"
    "${CURL_DIR}/lib/transfer.h"
    "${CURL_DIR}/lib/sha256.c"
    "${CURL_DIR}/lib/system_win32.h"
    "${CURL_DIR}/lib/connect.c"
    "${CURL_DIR}/lib/curl_sha512_256.h"
    "${CURL_DIR}/lib/ws.c"
    "${CURL_DIR}/lib/ftp.c"
    "${CURL_DIR}/lib/md5.c"
    "${CURL_DIR}/lib/vauth/krb5_sspi.c"
    "${CURL_DIR}/lib/vauth/spnego_sspi.c"
    "${CURL_DIR}/lib/vauth/ntlm.c"
    "${CURL_DIR}/lib/vauth/gsasl.c"
    "${CURL_DIR}/lib/vauth/spnego_gssapi.c"
    "${CURL_DIR}/lib/vauth/digest.h"
    "${CURL_DIR}/lib/vauth/ntlm_sspi.c"
    "${CURL_DIR}/lib/vauth/vauth.c"
    "${CURL_DIR}/lib/vauth/oauth2.c"
    "${CURL_DIR}/lib/vauth/cram.c"
    "${CURL_DIR}/lib/vauth/cleartext.c"
    "${CURL_DIR}/lib/vauth/krb5_gssapi.c"
    "${CURL_DIR}/lib/vauth/digest.c"
    "${CURL_DIR}/lib/vauth/digest_sspi.c"
    "${CURL_DIR}/lib/vauth/vauth.h"
    "${CURL_DIR}/lib/file.c"
    "${CURL_DIR}/lib/cfilters.h"
    "${CURL_DIR}/lib/http_digest.c"
    "${CURL_DIR}/lib/cf-haproxy.h"
    "${CURL_DIR}/lib/cf-h1-proxy.c"
    "${CURL_DIR}/lib/curl_md5.h"
    "${CURL_DIR}/lib/urlapi-int.h"
    "${CURL_DIR}/lib/hsts.c"
    "${CURL_DIR}/lib/vtls/mbedtls.c"
    "${CURL_DIR}/lib/vtls/gtls.c"
    "${CURL_DIR}/lib/vtls/vtls.h"
    "${CURL_DIR}/lib/vtls/hostcheck.c"
    "${CURL_DIR}/lib/vtls/rustls.c"
    "${CURL_DIR}/lib/vtls/wolfssl.h"
    "${CURL_DIR}/lib/vtls/vtls_spack.h"
    "${CURL_DIR}/lib/vtls/schannel.c"
    "${CURL_DIR}/lib/vtls/openssl.h"
    "${CURL_DIR}/lib/vtls/vtls_scache.c"
    "${CURL_DIR}/lib/vtls/keylog.h"
    "${CURL_DIR}/lib/vtls/cipher_suite.h"
    "${CURL_DIR}/lib/vtls/schannel_verify.c"
    "${CURL_DIR}/lib/vtls/x509asn1.h"
    "${CURL_DIR}/lib/vtls/mbedtls_threadlock.h"
    "${CURL_DIR}/lib/vtls/vtls.c"
    "${CURL_DIR}/lib/vtls/hostcheck.h"
    "${CURL_DIR}/lib/vtls/gtls.h"
    "${CURL_DIR}/lib/vtls/mbedtls.h"
    "${CURL_DIR}/lib/vtls/vtls_int.h"
    "${CURL_DIR}/lib/vtls/schannel_int.h"
    "${CURL_DIR}/lib/vtls/rustls.h"
    "${CURL_DIR}/lib/vtls/cipher_suite.c"
    "${CURL_DIR}/lib/vtls/vtls_scache.h"
    "${CURL_DIR}/lib/vtls/keylog.c"
    "${CURL_DIR}/lib/vtls/openssl.c"
    "${CURL_DIR}/lib/vtls/vtls_spack.c"
    "${CURL_DIR}/lib/vtls/wolfssl.c"
    "${CURL_DIR}/lib/vtls/schannel.h"
    "${CURL_DIR}/lib/vtls/mbedtls_threadlock.c"
    "${CURL_DIR}/lib/vtls/x509asn1.c"
    "${CURL_DIR}/lib/parsedate.h"
    "${CURL_DIR}/lib/curl_trc.h"
    "${CURL_DIR}/lib/socks.h"
    "${CURL_DIR}/lib/vssh/libssh.c"
    "${CURL_DIR}/lib/vssh/libssh2.c"
    "${CURL_DIR}/lib/vssh/curl_path.h"
    "${CURL_DIR}/lib/vssh/ssh.h"
    "${CURL_DIR}/lib/vssh/curl_path.c"
    "${CURL_DIR}/lib/vssh/wolfssh.c"
    "${CURL_DIR}/lib/asyn-ares.c"
    "${CURL_DIR}/lib/imap.c"
    "${CURL_DIR}/lib/headers.c"
    "${CURL_DIR}/lib/macos.c"
    "${CURL_DIR}/lib/uint-spbset.h"
    "${CURL_DIR}/lib/cookie.c"
    "${CURL_DIR}/lib/curl_krb5.h"
    "${CURL_DIR}/lib/hostip6.c"
    "${CURL_DIR}/lib/sendf.c"
    "${CURL_DIR}/lib/ftplistparser.c"
    "${CURL_DIR}/lib/http_ntlm.c"
    "${CURL_DIR}/lib/http_proxy.h"
    "${CURL_DIR}/lib/bufq.h"
    "${CURL_DIR}/lib/hash.h"
    "${CURL_DIR}/lib/config-mac.h"
    "${CURL_DIR}/lib/setopt.c"
    "${CURL_DIR}/lib/cw-pause.c"
    "${CURL_DIR}/lib/easy_lock.h"
    "${CURL_DIR}/lib/sigpipe.h"
    "${CURL_DIR}/lib/fake_addrinfo.c"
    "${CURL_DIR}/lib/getinfo.c"
    "${CURL_DIR}/lib/speedcheck.c"
    "${CURL_DIR}/lib/http1.h"
    "${CURL_DIR}/lib/cw-out.c"
    "${CURL_DIR}/lib/curl_rtmp.h"
    "${CURL_DIR}/lib/curl_gssapi.c"
    "${CURL_DIR}/lib/curl_fnmatch.c"
    "${CURL_DIR}/lib/select.h")

  if(HYDRA_COMPILER_MSVC)
    target_compile_options(curl PRIVATE /W3 /MP /wd4996 /GS-)
    target_compile_definitions(curl PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(curl PUBLIC _SSIZE_T_DEFINED)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      target_compile_definitions(curl PUBLIC "ssize_t=__int64")
    else()
      target_compile_definitions(curl PUBLIC "ssize_t=int")
    endif()
  else()
    target_compile_options(curl PRIVATE
      -Wall
      -Wextra
      -Wpedantic
      -Werror
      -Wdouble-promotion
      -Wfloat-equal
      -Wformat=2
      -Wmissing-declarations
      -Wwrite-strings
      -Wno-cast-align
      -Wno-cast-qual
      -Wno-unused-parameter
      -Wno-sign-conversion
      -Wno-sign-compare
      -Wno-enum-conversion
      -Wno-implicit-function-declaration
      -Wno-int-conversion
      -Wno-deprecated-declarations
      -Wno-strict-overflow
      -Wno-format-nonliteral)

    if(HYDRA_COMPILER_CLANG)
      target_compile_options(curl PRIVATE
        -Wno-implicit-int-conversion
        -Wno-documentation-deprecated-sync
        -Wno-conditional-uninitialized)
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_compile_options(curl PRIVATE
        -funroll-loops
        -fstrict-aliasing
        -ftree-vectorize
        -fno-math-errno
        -fwrapv)
    endif()
  endif()

  target_compile_definitions(curl PRIVATE BUILDING_LIBCURL)
  target_compile_definitions(curl PRIVATE UNICODE)
  target_compile_definitions(curl PRIVATE _UNICODE)
  target_compile_definitions(curl PRIVATE HTTP_ONLY)
  target_compile_definitions(curl PRIVATE ENABLE_IPV6)
  target_compile_definitions(curl PRIVATE SIZEOF_CURL_OFF_T=8)

  if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(curl PUBLIC CURL_STATICLIB)
  endif()

  target_compile_definitions(curl PRIVATE HAVE_LIBZ)
  target_link_libraries(curl PRIVATE ZLIB::ZLIB)

  target_compile_definitions(curl PRIVATE USE_NGHTTP2)
  target_link_libraries(curl PRIVATE Nghttp2::nghttp2)

  target_compile_definitions(curl PRIVATE USE_LIBPSL)
  target_link_libraries(curl PRIVATE PSL::psl)

  if(WIN32)
    target_compile_definitions(curl PRIVATE CURL_OS="Windows")
    target_compile_definitions(curl PRIVATE USE_SCHANNEL)
    target_compile_definitions(curl PRIVATE USE_WINDOWS_SSPI)
    target_compile_definitions(curl PRIVATE USE_THREADS_WIN32)
    target_link_libraries(curl PRIVATE ws2_32)
    target_link_libraries(curl PRIVATE Crypt32)
    target_link_libraries(curl PRIVATE Secur32)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "MSYS")
    target_compile_definitions(curl PRIVATE CURL_OS="MSYS")
    target_compile_definitions(curl PRIVATE CURL_CA_BUNDLE="/usr/ssl/certs/ca-bundle.crt")
    target_compile_definitions(curl PRIVATE CURL_CA_PATH="/usr/ssl/certs")
    target_compile_definitions(curl PRIVATE USE_MBEDTLS)
    target_compile_definitions(curl PRIVATE USE_THREADS_POSIX)
    target_compile_definitions(curl PRIVATE _POSIX_C_SOURCE=200809L)
    target_compile_definitions(curl PRIVATE HAVE_SYS_TIME_H)
    target_compile_definitions(curl PRIVATE HAVE_LONGLONG)
    target_compile_definitions(curl PRIVATE HAVE_RECV)
    target_compile_definitions(curl PRIVATE HAVE_SEND)
    target_compile_definitions(curl PRIVATE HAVE_SOCKET)
    target_compile_definitions(curl PRIVATE HAVE_NETDB_H)
    target_compile_definitions(curl PRIVATE HAVE_ARPA_INET_H)
    target_compile_definitions(curl PRIVATE HAVE_SNPRINTF)
    target_compile_definitions(curl PRIVATE HAVE_UNISTD_H)
    target_compile_definitions(curl PRIVATE HAVE_SYS_STAT_H)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_H)
    target_compile_definitions(curl PRIVATE HAVE_SELECT)
    target_compile_definitions(curl PRIVATE HAVE_POLL)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_O_NONBLOCK)
    target_compile_definitions(curl PRIVATE HAVE_STRUCT_TIMEVAL)
    target_compile_definitions(curl PRIVATE HAVE_GETSOCKNAME)
    target_compile_definitions(curl PRIVATE HAVE_GETADDRINFO)
    target_compile_definitions(curl PRIVATE HAVE_GETADDRINFO_THREADSAFE)
    target_compile_definitions(curl PRIVATE HAVE_FREEADDRINFO)
    target_compile_definitions(curl PRIVATE HAVE_SYS_TYPES_H)
    target_compile_definitions(curl PRIVATE HAVE_SYS_SOCKET_H)
    target_compile_definitions(curl PRIVATE HAVE_NETINET_IN_H)
    target_compile_definitions(curl PRIVATE HAVE_PTHREAD_H)
    target_link_libraries(curl PRIVATE MbedTLS::mbedtls)
    target_link_libraries(curl PRIVATE pthread)
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(curl PRIVATE CURL_OS="Linux")
    target_compile_definitions(curl PRIVATE CURL_CA_BUNDLE="/etc/ssl/certs/ca-certificates.crt")
    target_compile_definitions(curl PRIVATE CURL_CA_PATH="/etc/ssl/certs")
    target_compile_definitions(curl PRIVATE USE_MBEDTLS)
    target_compile_definitions(curl PRIVATE USE_ARES)
    target_compile_definitions(curl PRIVATE HAVE_SYS_TIME_H)
    target_compile_definitions(curl PRIVATE HAVE_LONGLONG)
    target_compile_definitions(curl PRIVATE HAVE_RECV)
    target_compile_definitions(curl PRIVATE HAVE_SEND)
    target_compile_definitions(curl PRIVATE HAVE_SOCKET)
    target_compile_definitions(curl PRIVATE HAVE_NETDB_H)
    target_compile_definitions(curl PRIVATE HAVE_ARPA_INET_H)
    target_compile_definitions(curl PRIVATE HAVE_SNPRINTF)
    target_compile_definitions(curl PRIVATE HAVE_UNISTD_H)
    target_compile_definitions(curl PRIVATE HAVE_SYS_STAT_H)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_H)
    target_compile_definitions(curl PRIVATE HAVE_SELECT)
    target_compile_definitions(curl PRIVATE HAVE_POLL)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_O_NONBLOCK)
    target_compile_definitions(curl PRIVATE HAVE_STRUCT_TIMEVAL)
    target_compile_definitions(curl PRIVATE HAVE_GETSOCKNAME)
    target_compile_definitions(curl PRIVATE _POSIX_C_SOURCE=200809L)
    target_link_libraries(curl PRIVATE MbedTLS::mbedtls)
    target_link_libraries(curl PRIVATE c-ares::cares)
  elseif(APPLE)
    target_compile_definitions(curl PRIVATE CURL_OS="Darwin")
    target_compile_definitions(curl PRIVATE CURL_CA_BUNDLE="/etc/ssl/cert.pem")
    target_compile_definitions(curl PRIVATE CURL_CA_PATH="/etc/ssl/certs")
    target_compile_definitions(curl PRIVATE USE_MBEDTLS)
    target_compile_definitions(curl PRIVATE USE_ARES)
    target_compile_definitions(curl PRIVATE HAVE_RECV)
    target_compile_definitions(curl PRIVATE HAVE_SEND)
    target_compile_definitions(curl PRIVATE HAVE_SOCKET)
    target_compile_definitions(curl PRIVATE HAVE_NETDB_H)
    target_compile_definitions(curl PRIVATE HAVE_SNPRINTF)
    target_compile_definitions(curl PRIVATE HAVE_UNISTD_H)
    target_compile_definitions(curl PRIVATE HAVE_SYS_STAT_H)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_H)
    target_compile_definitions(curl PRIVATE HAVE_SELECT)
    target_compile_definitions(curl PRIVATE HAVE_POLL)
    target_compile_definitions(curl PRIVATE HAVE_FCNTL_O_NONBLOCK)
    target_compile_definitions(curl PRIVATE HAVE_STRUCT_TIMEVAL)
    target_compile_definitions(curl PRIVATE HAVE_GETSOCKNAME)
    target_link_libraries(curl PRIVATE MbedTLS::mbedtls)
    target_link_libraries(curl PRIVATE c-ares::cares)
    target_link_libraries(curl PRIVATE "-framework Foundation")
    target_link_libraries(curl PRIVATE "-framework SystemConfiguration")
  endif()

  target_include_directories(curl PRIVATE "${CURL_DIR}/lib")
  target_include_directories(curl PUBLIC
    "$<BUILD_INTERFACE:${CURL_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  add_library(CURL::libcurl ALIAS curl)

  set_target_properties(curl
    PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      POSITION_INDEPENDENT_CODE ON
      OUTPUT_NAME curl
      PUBLIC_HEADER "${CURL_PUBLIC_HEADER}"
      PRIVATE_HEADER "${CURL_PRIVATE_HEADERS}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      EXPORT_NAME curl)

  if(SOURCEMETA_HYDRA_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS curl
      EXPORT curl
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/curl"
        COMPONENT sourcemeta_hydra_dev
      PRIVATE_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/curl"
        COMPONENT sourcemeta_hydra_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_hydra
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra
        NAMELINK_COMPONENT sourcemeta_hydra_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_hydra_dev)
    install(EXPORT curl
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/curl"
      COMPONENT sourcemeta_hydra_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/curl-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/curl.cmake\")\n"
      "check_required_components(\"curl\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/curl-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/curl"
      COMPONENT sourcemeta_hydra_dev)
  endif()

  set(CURL_FOUND ON)
endif()
