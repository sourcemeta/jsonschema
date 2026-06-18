#include <sourcemeta/core/http.h>

#ifdef SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL
#include <curl/curl.h> // curl_easy_*, curl_slist_*, curl_global_init, CURLOPT_*
#endif

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

#ifndef SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL
#include <dlfcn.h> // dlopen, dlsym, dlerror, RTLD_NOW

#include <array>   // std::array
#include <cstdlib> // std::getenv
#include <cstring> // std::memcpy
#include <vector>  // std::vector

// When cURL is not linked at build time, we load it at runtime via dlopen
// and therefore need no curl headers. The following reproduces the small
// subset of libcurl's C API this backend uses. Every type, prototype, and
// option id is part of libcurl's frozen ABI (SONAME libcurl.so.4, stable
// since 2006), so these values never change. The prototypes are never
// called directly (we invoke them through dlsym'd pointers) nor linked;
// they exist only so the shared CurlApi table can derive their types
extern "C" {

using CURL = void;
using CURLcode = int;
using CURLoption = int;
using CURLINFO = int;
using curl_off_t = long long;

struct curl_slist;

auto curl_global_init(long flags) -> CURLcode;
auto curl_easy_init() -> CURL *;
auto curl_easy_cleanup(CURL *handle) -> void;
auto curl_easy_setopt(CURL *handle, CURLoption option, ...) -> CURLcode;
auto curl_easy_perform(CURL *handle) -> CURLcode;
auto curl_easy_getinfo(CURL *handle, CURLINFO info, ...) -> CURLcode;
auto curl_easy_strerror(CURLcode code) -> const char *;
auto curl_slist_append(curl_slist *list, const char *value) -> curl_slist *;
auto curl_slist_free_all(curl_slist *list) -> void;

} // extern "C"

// Option ids are a type-class base plus an index in libcurl's headers; the
// resolved values are reproduced here (see the trailing comments)
constexpr CURLcode CURLE_OK{0};
constexpr long CURL_GLOBAL_ALL{3};                   // SSL(1<<0) | WIN32(1<<1)
constexpr CURLoption CURLOPT_URL{10002};             // STRINGPOINT + 2
constexpr CURLoption CURLOPT_FOLLOWLOCATION{52};     // LONG + 52
constexpr CURLoption CURLOPT_MAXREDIRS{68};          // LONG + 68
constexpr CURLoption CURLOPT_NOSIGNAL{99};           // LONG + 99
constexpr CURLoption CURLOPT_ACCEPT_ENCODING{10102}; // STRINGPOINT + 102
constexpr CURLoption CURLOPT_TIMEOUT_MS{155};        // LONG + 155
constexpr CURLoption CURLOPT_CONNECTTIMEOUT_MS{156}; // LONG + 156
constexpr CURLoption CURLOPT_WRITEFUNCTION{20011};   // FUNCTIONPOINT + 11
constexpr CURLoption CURLOPT_WRITEDATA{10001};       // CBPOINT + 1
constexpr CURLoption CURLOPT_HEADERFUNCTION{20079};  // FUNCTIONPOINT + 79
constexpr CURLoption CURLOPT_HEADERDATA{10029};      // CBPOINT + 29
constexpr CURLoption CURLOPT_POSTFIELDSIZE_LARGE{30120}; // OFF_T + 120
constexpr CURLoption CURLOPT_POSTFIELDS{10015};          // OBJECTPOINT + 15
constexpr CURLoption CURLOPT_HTTPHEADER{10023};          // SLISTPOINT + 23
constexpr CURLoption CURLOPT_NOBODY{44};                 // LONG + 44
constexpr CURLoption CURLOPT_CUSTOMREQUEST{10036};       // STRINGPOINT + 36
constexpr CURLINFO CURLINFO_RESPONSE_CODE{2097154}; // CURLINFO_LONG(0x200000)+2
constexpr CURLINFO CURLINFO_EFFECTIVE_URL{
    1048577}; // CURLINFO_STRING(0x100000)+1
#endif

namespace {

constexpr std::string_view HTTP_RESPONSE_TOO_LARGE_MESSAGE{
    "The response exceeds the maximum allowed size"};

// The subset of the libcurl C API this backend relies on, captured as
// function pointers so the request logic is shared between the link-time
// backend (SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL) and the default
// runtime-loaded (dlopen) backend. Member types are derived from the curl
// headers, so they stay in sync with the real prototypes
struct CurlApi {
  decltype(&curl_global_init) global_init;
  decltype(&curl_easy_init) easy_init;
  decltype(&curl_easy_cleanup) easy_cleanup;
  decltype(&curl_easy_setopt) easy_setopt;
  decltype(&curl_easy_perform) easy_perform;
  decltype(&curl_easy_getinfo) easy_getinfo;
  decltype(&curl_easy_strerror) easy_strerror;
  decltype(&curl_slist_append) slist_append;
  decltype(&curl_slist_free_all) slist_free_all;
};

class CurlHandle {
public:
  explicit CurlHandle(const CurlApi &api)
      : api_{api}, handle_{api.easy_init()} {}
  ~CurlHandle() {
    if (this->handle_) {
      this->api_.easy_cleanup(this->handle_);
    }
  }

  CurlHandle(const CurlHandle &) = delete;
  auto operator=(const CurlHandle &) -> CurlHandle & = delete;
  CurlHandle(CurlHandle &&) = delete;
  auto operator=(CurlHandle &&) -> CurlHandle & = delete;

  [[nodiscard]] auto get() const -> CURL * { return this->handle_; }
  explicit operator bool() const { return this->handle_ != nullptr; }

private:
  const CurlApi &api_;
  CURL *handle_;
};

class CurlHeaderList {
public:
  explicit CurlHeaderList(const CurlApi &api) : api_{api} {}
  ~CurlHeaderList() {
    if (this->list_) {
      this->api_.slist_free_all(this->list_);
    }
  }

  CurlHeaderList(const CurlHeaderList &) = delete;
  auto operator=(const CurlHeaderList &) -> CurlHeaderList & = delete;
  CurlHeaderList(CurlHeaderList &&) = delete;
  auto operator=(CurlHeaderList &&) -> CurlHeaderList & = delete;

  auto append(const std::string &line) -> void {
    auto *result{this->api_.slist_append(this->list_, line.c_str())};
    if (result) {
      this->list_ = result;
    }
  }

  [[nodiscard]] auto get() const -> curl_slist * { return this->list_; }

private:
  const CurlApi &api_;
  curl_slist *list_{nullptr};
};

struct BodyContext {
  std::string *output;
  std::optional<std::size_t> maximum_size;
  bool maximum_size_exceeded{false};
};

auto body_callback(char *data, std::size_t size, std::size_t count,
                   void *user_data) -> std::size_t {
  auto *context{static_cast<BodyContext *>(user_data)};
  const std::size_t chunk{size * count};
  if (context->maximum_size.has_value() &&
      (context->output->size() > context->maximum_size.value() ||
       chunk > context->maximum_size.value() - context->output->size())) {
    context->maximum_size_exceeded = true;
    // Returning a smaller count than given aborts the transfer
    return 0;
  }

  context->output->append(data, chunk);
  return chunk;
}

auto header_callback(char *data, std::size_t size, std::size_t count,
                     void *output) -> std::size_t {
  sourcemeta::core::http_accumulate_header_line(
      *static_cast<std::string *>(output),
      std::string_view{data, size * count});
  return size * count;
}

#ifndef SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL

using sourcemeta::core::HTTPSystemBackendError;

constexpr std::string_view CURL_LIBRARY_ENV{"SOURCEMETA_CORE_CURL_SO"};

// Tried in order. Every entry carries the `.so.4` SONAME so we only ever
// bind an ABI-compatible cURL (never the unversioned `libcurl.so` dev
// symlink, which could point at a different major version). The bare
// soname is first because it resolves through the dynamic linker (ld.so
// cache on glibc, default /lib:/usr/lib on musl) and is present on every
// mainstream distribution. The absolute entries are fallbacks for
// environments where the cache is absent (custom prefixes, ldconfig not
// run). The trailing GnuTLS entry is an ABI-compatible last resort for
// minimal Debian and Ubuntu systems that ship only that build
constexpr std::array<std::string_view, 10> CURL_CANDIDATE_PATHS{
    {"libcurl.so.4", "/usr/lib/x86_64-linux-gnu/libcurl.so.4",
     "/usr/lib/aarch64-linux-gnu/libcurl.so.4",
     "/usr/lib/arm-linux-gnueabihf/libcurl.so.4",
     "/usr/lib/i386-linux-gnu/libcurl.so.4", "/usr/lib64/libcurl.so.4",
     "/lib64/libcurl.so.4", "/usr/lib/libcurl.so.4",
     "/usr/local/lib/libcurl.so.4", "libcurl-gnutls.so.4"}};

struct ResolvedLibrary {
  void *handle;
  std::string path;
};

template <typename Signature>
auto resolve_symbol(const ResolvedLibrary &library, const char *name)
    -> Signature {
  // Clear any stale error, then distinguish a null symbol from a null value
  dlerror();
  void *symbol{dlsym(library.handle, name)};
  if (dlerror() != nullptr) {
    throw HTTPSystemBackendError{
        "The cURL library was loaded but does not provide the expected API",
        std::string{CURL_LIBRARY_ENV},
        {library.path}};
  }

  // Copy the pointer representation instead of reinterpret_cast, which the
  // standard only conditionally supports for object-to-function conversions.
  // POSIX guarantees dlsym results are convertible to function pointers
  Signature function{};
  std::memcpy(&function, &symbol, sizeof(function));
  return function;
}

auto open_library() -> ResolvedLibrary {
  if (const auto *configured_path{std::getenv(CURL_LIBRARY_ENV.data())};
      configured_path != nullptr && configured_path[0] != '\0') {
    if (auto *handle{dlopen(configured_path, RTLD_NOW)}; handle != nullptr) {
      return {handle, configured_path};
    }

    throw HTTPSystemBackendError{
        "Could not load the cURL library from the configured path",
        std::string{CURL_LIBRARY_ENV},
        {std::string{configured_path}}};
  }

  for (const auto candidate : CURL_CANDIDATE_PATHS) {
    if (auto *handle{dlopen(candidate.data(), RTLD_NOW)}; handle != nullptr) {
      return {handle, std::string{candidate}};
    }
  }

  std::vector<std::string> searched;
  searched.reserve(CURL_CANDIDATE_PATHS.size());
  for (const auto candidate : CURL_CANDIDATE_PATHS) {
    searched.emplace_back(candidate);
  }

  throw HTTPSystemBackendError{
      "Could not find the system cURL library (libcurl)",
      std::string{CURL_LIBRARY_ENV}, std::move(searched)};
}

auto load_curl() -> const CurlApi & {
  // The handle is intentionally never dlclose()d: the function pointers
  // must remain valid for the lifetime of the process
  static const ResolvedLibrary library{open_library()};
  static const CurlApi api{
      .global_init = resolve_symbol<decltype(CurlApi::global_init)>(
          library, "curl_global_init"),
      .easy_init = resolve_symbol<decltype(CurlApi::easy_init)>(
          library, "curl_easy_init"),
      .easy_cleanup = resolve_symbol<decltype(CurlApi::easy_cleanup)>(
          library, "curl_easy_cleanup"),
      .easy_setopt = resolve_symbol<decltype(CurlApi::easy_setopt)>(
          library, "curl_easy_setopt"),
      .easy_perform = resolve_symbol<decltype(CurlApi::easy_perform)>(
          library, "curl_easy_perform"),
      .easy_getinfo = resolve_symbol<decltype(CurlApi::easy_getinfo)>(
          library, "curl_easy_getinfo"),
      .easy_strerror = resolve_symbol<decltype(CurlApi::easy_strerror)>(
          library, "curl_easy_strerror"),
      .slist_append = resolve_symbol<decltype(CurlApi::slist_append)>(
          library, "curl_slist_append"),
      .slist_free_all = resolve_symbol<decltype(CurlApi::slist_free_all)>(
          library, "curl_slist_free_all")};
  return api;
}

#endif

auto acquire_api() -> const CurlApi & {
#ifdef SOURCEMETA_CORE_HTTP_USE_SYSTEM_CURL
  static const CurlApi api{.global_init = &curl_global_init,
                           .easy_init = &curl_easy_init,
                           .easy_cleanup = &curl_easy_cleanup,
                           .easy_setopt = &curl_easy_setopt,
                           .easy_perform = &curl_easy_perform,
                           .easy_getinfo = &curl_easy_getinfo,
                           .easy_strerror = &curl_easy_strerror,
                           .slist_append = &curl_slist_append,
                           .slist_free_all = &curl_slist_free_all};
  return api;
#else
  return load_curl();
#endif
}

} // namespace

namespace sourcemeta::core {

auto HTTPSystemRequest::send() const -> HTTPResponse {
  const CurlApi &api{acquire_api()};

  static const CURLcode global_initialization{api.global_init(CURL_GLOBAL_ALL)};
  if (global_initialization != CURLE_OK) {
    throw HTTPError{this->method_, this->url_,
                    api.easy_strerror(global_initialization)};
  }

  const CurlHandle handle{api};
  if (!handle) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to initialise the HTTP client"};
  }

  HTTPResponse response;
  api.easy_setopt(handle.get(), CURLOPT_URL, this->url_.c_str());
  api.easy_setopt(handle.get(), CURLOPT_FOLLOWLOCATION,
                  this->follow_redirects_ ? 1L : 0L);
  if (this->follow_redirects_) {
    api.easy_setopt(handle.get(), CURLOPT_MAXREDIRS,
                    static_cast<long>(this->maximum_redirects_));
  }

  api.easy_setopt(handle.get(), CURLOPT_NOSIGNAL, 1L);
  api.easy_setopt(handle.get(), CURLOPT_TIMEOUT_MS,
                  static_cast<long>(this->timeout_.count()));
  if (this->connect_timeout_.has_value()) {
    api.easy_setopt(handle.get(), CURLOPT_CONNECTTIMEOUT_MS,
                    static_cast<long>(this->connect_timeout_.value().count()));
  }

  // Advertise and transparently decode all supported content encodings,
  // matching what the NSURLSession and WinHTTP backends do
  api.easy_setopt(handle.get(), CURLOPT_ACCEPT_ENCODING, "");

  std::string raw_headers;
  BodyContext body_context{.output = &response.body,
                           .maximum_size = this->maximum_response_size_};
  api.easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, body_callback);
  api.easy_setopt(handle.get(), CURLOPT_WRITEDATA, &body_context);
  api.easy_setopt(handle.get(), CURLOPT_HEADERFUNCTION, header_callback);
  api.easy_setopt(handle.get(), CURLOPT_HEADERDATA, &raw_headers);

  CurlHeaderList header_list{api};
  for (const auto &[name, value] : this->headers_) {
    std::string line{name};
    // The semicolon form is how cURL distinguishes a header with an
    // empty value from a header to suppress
    if (value.empty()) {
      line += ";";
    } else {
      line += ": ";
      line += value;
    }

    header_list.append(line);
  }

  if (this->body_.has_value()) {
    std::string content_type_line{"Content-Type: "};
    content_type_line += this->body_.value().content_type;
    header_list.append(content_type_line);
    api.easy_setopt(handle.get(), CURLOPT_POSTFIELDSIZE_LARGE,
                    static_cast<curl_off_t>(this->body_.value().data.size()));
    api.easy_setopt(handle.get(), CURLOPT_POSTFIELDS,
                    this->body_.value().data.data());
  }

  if (header_list.get()) {
    api.easy_setopt(handle.get(), CURLOPT_HTTPHEADER, header_list.get());
  }

  const std::string method{http_method_string(this->method_)};
  if (this->method_ == HTTPMethod::HEAD) {
    api.easy_setopt(handle.get(), CURLOPT_NOBODY, 1L);
  } else if (this->method_ != HTTPMethod::GET || this->body_.has_value()) {
    api.easy_setopt(handle.get(), CURLOPT_CUSTOMREQUEST, method.c_str());
  }

  const auto code{api.easy_perform(handle.get())};
  if (code != CURLE_OK) {
    if (body_context.maximum_size_exceeded) {
      throw HTTPError{this->method_, this->url_,
                      std::string{HTTP_RESPONSE_TOO_LARGE_MESSAGE}};
    }

    throw HTTPError{this->method_, this->url_, api.easy_strerror(code)};
  }

  long status_code{0};
  api.easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &status_code);
  char *effective_url{nullptr};
  api.easy_getinfo(handle.get(), CURLINFO_EFFECTIVE_URL, &effective_url);
  if (effective_url != nullptr) {
    response.url.assign(effective_url);
  }

  http_parse_headers(raw_headers, response.headers);
  response.status =
      http_status_from_code(static_cast<std::uint16_t>(status_code));
  return response;
}

} // namespace sourcemeta::core
