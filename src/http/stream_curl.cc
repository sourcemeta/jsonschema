#include <sourcemeta/jsonschema/http_error.h>
#include <sourcemeta/jsonschema/http_method.h>
#include <sourcemeta/jsonschema/http_status.h>
#include <sourcemeta/jsonschema/http_stream.h>

#include <curl/curl.h>

#include <algorithm>    // std::transform, std::copy
#include <cassert>      // assert
#include <cctype>       // std::tolower
#include <charconv>     // std::from_chars
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint64_t
#include <iterator>     // std::back_inserter
#include <memory>       // std::make_unique
#include <optional>     // std::optional
#include <sstream>      // std::stringstream, std::ostringstream
#include <string>       // std::string
#include <system_error> // std::errc
#include <type_traits>  // std::underlying_type_t
#include <utility>      // std::move

// The internal implementation of the cURL backend
namespace sourcemeta::jsonschema::http {
struct ClientStream::Internal {
  CURL *handle{nullptr};
  struct curl_slist *headers{nullptr};
  std::string url;
  DataCallback on_data;
  HeaderCallback on_header;
  BodyCallback on_body;
  Method method{Method::GET};
  std::optional<Status> status;
  static std::uint64_t count;
};

std::uint64_t ClientStream::Internal::count = 0;
} // namespace sourcemeta::jsonschema::http

namespace {
inline auto handle_curl(CURLcode code) -> void {
  if (code != CURLE_OK) {
    throw sourcemeta::jsonschema::http::Error(curl_easy_strerror(code));
  }
}

auto callback_on_response_body(
    const void *const data, const std::size_t size, const std::size_t count,
    const sourcemeta::jsonschema::http::ClientStream *const request) noexcept
    -> std::size_t {
  const std::size_t total_size{size * count};
  if (request->internal->on_data) {
    try {
      assert(request->internal->status.has_value());
      request->internal->on_data(
          request->internal->status.value(),
          {reinterpret_cast<const std::uint8_t *>(data), total_size});
    } catch (...) {
      return 0;
    }
  }

  return total_size;
}

auto callback_on_header(
    const void *const data, const std::size_t size, const std::size_t count,
    sourcemeta::jsonschema::http::ClientStream *const request) -> std::size_t {
  const std::size_t total_size{size * count};
  const std::string_view line{static_cast<const char *>(data), total_size};
  const std::size_t colon{line.find(':')};

  if (colon == line.npos) {
    // Avoid registering statuses twice
    if (request->internal->status.has_value()) {
      return total_size;
    }

    std::underlying_type_t<sourcemeta::jsonschema::http::Status> code{0};
    if (line.starts_with("HTTP/2 ")) {
      if (std::from_chars(line.data() + 7, line.data() + 10, code).ec !=
          std::errc()) {
        return 0;
      }
    } else if (line.starts_with("HTTP/1.1 ")) {
      if (std::from_chars(line.data() + 9, line.data() + 12, code).ec !=
          std::errc()) {
        return 0;
      }
    } else {
      return 0;
    }

    try {
      assert(code > 0);
      request->internal->status.emplace(
          sourcemeta::jsonschema::http::Status{code});
      return total_size;
    } catch (...) {
      return 0;
    }
  }

  // Parse header
  constexpr std::string_view spacing{" \t\r\n"};
  const auto key_start{line.find_first_not_of(spacing)};
  const auto key_end{line.find_last_not_of(spacing, colon)};
  const auto value_start{line.find_first_not_of(spacing, colon + 1)};
  const auto value_end{line.find_last_not_of(spacing) + 1};
  const auto key{line.substr(key_start, key_end - key_start)};

  if (request->internal->on_header) {
    try {
      assert(request->internal->status.has_value());

      // Convert headers to lowercase
      std::string key_lowercase;
      std::ranges::transform(
          key, std::back_inserter(key_lowercase), [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
          });

      request->internal->on_header(
          request->internal->status.value(), key_lowercase,
          line.substr(value_start, value_end - value_start));
    } catch (...) {
      return 0;
    }
  }

  return total_size;
}

auto callback_on_request_body(
    char *buffer, const std::size_t size, const std::size_t count,
    sourcemeta::jsonschema::http::ClientStream *const request) noexcept
    -> std::size_t {
  assert(buffer);
  assert(request->internal->on_body);
  const std::size_t total_size{size * count};

  try {
    const auto bytes{request->internal->on_body(total_size)};
    assert(bytes.size() <= total_size);
    std::ranges::copy(bytes, buffer);
    return bytes.size();
  } catch (...) {
    // The read callback may return CURL_READFUNC_ABORT to stop the current
    // operation immediately, resulting in a CURLE_ABORTED_BY_CALLBACK error
    // code from the transfer. See
    // https://curl.se/libcurl/c/CURLOPT_READFUNCTION.html
    return CURL_READFUNC_ABORT;
  }
}

} // namespace

namespace sourcemeta::jsonschema::http {

ClientStream::ClientStream(std::string url)
    : internal{std::make_unique<ClientStream::Internal>()} {
  // Globally initialize cURL
  if (this->internal->count == 0) {
    handle_curl(curl_global_init(CURL_GLOBAL_DEFAULT));
  }
  this->internal->count += 1;

  // Initialize request
  this->internal->handle = curl_easy_init();
  if (!this->internal->handle) {
    throw Error("Failed to initialize cURL");
  }

  this->internal->url = std::move(url);
}

ClientStream::ClientStream(ClientStream &&other) noexcept
    : internal{std::make_unique<ClientStream::Internal>()} {
  this->internal->handle = other.internal->handle;
  this->internal->headers = other.internal->headers;
  this->internal->url = std::move(other.internal->url);
  this->internal->on_data = other.internal->on_data;
  this->internal->on_header = other.internal->on_header;
  this->internal->on_body = other.internal->on_body;
  this->internal->method = other.internal->method;
  this->internal->status = other.internal->status;
  other.internal->handle = nullptr;
  other.internal->headers = nullptr;
  other.internal->on_data = nullptr;
  other.internal->on_header = nullptr;
  other.internal->on_body = nullptr;
  this->internal->count -= 1;
}

auto ClientStream::operator=(ClientStream &&other) noexcept -> ClientStream & {
  if (this == &other) {
    return *this;
  }

  this->internal->handle = other.internal->handle;
  this->internal->headers = other.internal->headers;
  this->internal->url = std::move(other.internal->url);
  this->internal->on_data = other.internal->on_data;
  this->internal->on_header = other.internal->on_header;
  this->internal->on_body = other.internal->on_body;
  this->internal->method = other.internal->method;
  this->internal->status = other.internal->status;
  other.internal->handle = nullptr;
  other.internal->headers = nullptr;
  other.internal->on_data = nullptr;
  other.internal->on_header = nullptr;
  other.internal->on_body = nullptr;
  this->internal->count -= 1;
  return *this;
}

ClientStream::~ClientStream() {
  curl_slist_free_all(this->internal->headers);
  curl_easy_cleanup(this->internal->handle);
  this->internal->count -= 1;
  if (this->internal->count == 0) {
    curl_global_cleanup();
  }
}

auto ClientStream::method(const Method method) noexcept -> void {
  this->internal->method = method;
}

auto ClientStream::method() const noexcept -> Method {
  return this->internal->method;
}

auto ClientStream::on_data(DataCallback callback) noexcept -> void {
  this->internal->on_data = std::move(callback);
}

auto ClientStream::on_header(HeaderCallback callback) noexcept -> void {
  this->internal->on_header = std::move(callback);
}

auto ClientStream::on_body(BodyCallback callback) noexcept -> void {
  this->internal->on_body = std::move(callback);
}

auto ClientStream::header(std::string_view key, std::string_view value)
    -> void {
  std::stringstream stream;
  stream << key << ": " << value;
  const std::string result{stream.str()};
  this->internal->headers =
      curl_slist_append(this->internal->headers, result.c_str());
}

auto ClientStream::header(std::string_view key, int value) -> void {
  this->header(key, std::to_string(value));
}

auto ClientStream::url() const -> std::string_view {
  return this->internal->url;
}

auto ClientStream::send() -> Status {
  switch (this->internal->method) {
    case Method::GET:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "GET"));
      break;
    case Method::HEAD:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "HEAD"));
      break;
    case Method::POST:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "POST"));
      break;
    case Method::PUT:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "PUT"));
      break;

      // Can't find it in the docs, but seems like MSVC defines
      // a DELETE macro that confuses this clause.
#ifdef WIN32
#pragma push_macro("DELETE")
#undef DELETE
#endif
    case Method::DELETE:
#ifdef WIN32
#pragma pop_macro("DELETE")
#endif
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "DELETE"));
      break;
    case Method::CONNECT:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "CONNECT"));
      break;
    case Method::OPTIONS:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "OPTIONS"));
      break;
    case Method::TRACE:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "TRACE"));
      break;
    case Method::PATCH:
      handle_curl(curl_easy_setopt(this->internal->handle,
                                   CURLOPT_CUSTOMREQUEST, "PATCH"));
      break;
  }

  handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_URL,
                               this->internal->url.c_str()));

  // Accept all supported compression mechanisms
  // See https://curl.se/libcurl/c/CURLOPT_ACCEPT_ENCODING.html
  handle_curl(
      curl_easy_setopt(this->internal->handle, CURLOPT_ACCEPT_ENCODING, ""));

  // Follow re-directs
  // See https://curl.se/libcurl/c/CURLOPT_FOLLOWLOCATION.html
  handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_FOLLOWLOCATION,
                               CURLFOLLOW_ALL));

  // Otherwise cURL will hang for some seconds waiting for a response when
  // performing a HEAD request
  if (this->internal->method == Method::HEAD) {
    handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_NOBODY, 1L));
  } else {
    handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_WRITEFUNCTION,
                                 callback_on_response_body));
    handle_curl(
        curl_easy_setopt(this->internal->handle, CURLOPT_WRITEDATA, this));
  }

  if (this->internal->method != Method::HEAD && this->internal->on_body) {
    handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_READFUNCTION,
                                 callback_on_request_body));
    handle_curl(
        curl_easy_setopt(this->internal->handle, CURLOPT_READDATA, this));
    // Otherwise the read function is never called
    handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_UPLOAD, 1L));
    // Disable the default "Expect: 100-continue" header that cURL will
    // automatically add when enabling `CURLOPT_UPLOAD`.
    // See https://curl.se/libcurl/c/CURLOPT_UPLOAD.html
    // TODO: Let clients manually specify this header if they want
    this->internal->headers =
        curl_slist_append(this->internal->headers, "Expect:");
  }

  handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_HEADERFUNCTION,
                               callback_on_header));
  handle_curl(
      curl_easy_setopt(this->internal->handle, CURLOPT_HEADERDATA, this));
  handle_curl(curl_easy_setopt(this->internal->handle, CURLOPT_HTTPHEADER,
                               this->internal->headers));

  // Perform request
  handle_curl(curl_easy_perform(this->internal->handle));

  // Get status code
  long code{0};
  handle_curl(
      curl_easy_getinfo(this->internal->handle, CURLINFO_RESPONSE_CODE, &code));
  assert(code > 0);
  assert(this->internal->status.has_value());
  assert(static_cast<std::underlying_type_t<Status>>(code) ==
             static_cast<std::underlying_type_t<Status>>(
                 this->internal->status.value()) ||
         this->internal->status.value() == Status::MOVED_PERMANENTLY);
  return Status{static_cast<std::underlying_type_t<Status>>(code)};
}

} // namespace sourcemeta::jsonschema::http
