#include <sourcemeta/core/http.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h> // DWORD, GetLastError, LPVOID
#include <winhttp.h> // WinHttp*

// `windows.h` defines a `DELETE` macro that conflicts with
// `sourcemeta::core::HTTPMethod::DELETE`
#ifdef DELETE
#undef DELETE
#endif

#include <sourcemeta/core/unicode.h>

#include <chrono>      // std::chrono::milliseconds
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <limits>      // std::numeric_limits
#include <string>      // std::string, std::wstring
#include <string_view> // std::wstring_view
#include <utility>     // std::pair
#include <vector>      // std::vector

namespace {

constexpr std::string_view HTTP_RESPONSE_TOO_LARGE_MESSAGE{
    "The response exceeds the maximum allowed size"};

// WinHttpSetTimeouts takes signed millisecond counts where zero requests no
// timeout. Floor non-positive durations to the smallest bound so a misused
// value cannot become an unbounded wait, and saturate large ones to avoid a
// narrowing wrap
auto to_winhttp_timeout(const std::chrono::milliseconds value) -> int {
  if (value.count() <= 0) {
    return 1;
  }

  if (value.count() > std::numeric_limits<int>::max()) {
    return std::numeric_limits<int>::max();
  }

  return static_cast<int>(value.count());
}

class WinHTTPHandle {
public:
  WinHTTPHandle(const HINTERNET handle) : handle_{handle} {}
  ~WinHTTPHandle() {
    if (this->handle_) {
      WinHttpCloseHandle(this->handle_);
    }
  }

  WinHTTPHandle(const WinHTTPHandle &) = delete;
  auto operator=(const WinHTTPHandle &) -> WinHTTPHandle & = delete;
  WinHTTPHandle(WinHTTPHandle &&) = delete;
  auto operator=(WinHTTPHandle &&) -> WinHTTPHandle & = delete;

  auto get() const -> HINTERNET { return this->handle_; }
  explicit operator bool() const { return this->handle_ != nullptr; }

private:
  HINTERNET handle_;
};

auto parse_response_headers(
    const HINTERNET request,
    std::vector<std::pair<std::string, std::string>> &headers) -> void {
  DWORD size{0};
  WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                      WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                      &size, WINHTTP_NO_HEADER_INDEX);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return;
  }

  std::wstring buffer(size / sizeof(wchar_t), L'\0');
  if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                           WINHTTP_HEADER_NAME_BY_INDEX, buffer.data(), &size,
                           WINHTTP_NO_HEADER_INDEX)) {
    return;
  }

  sourcemeta::core::http_parse_headers(sourcemeta::core::wide_to_utf8(buffer),
                                       headers);
}

auto query_effective_url(const HINTERNET request) -> std::string {
  DWORD size{0};
  WinHttpQueryOption(request, WINHTTP_OPTION_URL, nullptr, &size);
  if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0) {
    return {};
  }

  std::wstring buffer(size / sizeof(wchar_t), L'\0');
  if (!WinHttpQueryOption(request, WINHTTP_OPTION_URL, buffer.data(), &size)) {
    return {};
  }

  buffer.resize(size / sizeof(wchar_t));
  if (!buffer.empty() && buffer.back() == L'\0') {
    buffer.pop_back();
  }

  return sourcemeta::core::wide_to_utf8(buffer);
}

} // namespace

namespace sourcemeta::core {

auto HTTPSystemRequest::send() const -> HTTPResponse {
  HTTPResponse response;

  const auto wide_url{sourcemeta::core::utf8_to_wide(this->url_)};
  URL_COMPONENTS components{};
  components.dwStructSize = sizeof(components);
  components.dwHostNameLength = static_cast<DWORD>(-1);
  components.dwUrlPathLength = static_cast<DWORD>(-1);
  components.dwExtraInfoLength = static_cast<DWORD>(-1);
  if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components)) {
    throw HTTPError{this->method_, this->url_, "Invalid URL"};
  }

  const std::wstring host{components.lpszHostName, components.dwHostNameLength};
  std::wstring path{components.lpszUrlPath, components.dwUrlPathLength};
  if (components.lpszExtraInfo) {
    // The fragment, if any, must never be sent to the server
    const std::wstring_view extra_information{components.lpszExtraInfo,
                                              components.dwExtraInfoLength};
    path.append(extra_information.substr(0, extra_information.find(L'#')));
  }

  const WinHTTPHandle session{
      WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
  if (!session) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to initialise the HTTP client"};
  }

  const WinHTTPHandle connection{
      WinHttpConnect(session.get(), host.c_str(), components.nPort, 0)};
  if (!connection) {
    throw HTTPError{this->method_, this->url_, "Failed to connect to the host"};
  }

  const auto secure{components.nScheme == INTERNET_SCHEME_HTTPS};
  const auto method{
      sourcemeta::core::utf8_to_wide(http_method_string(this->method_))};
  const WinHTTPHandle request_handle{WinHttpOpenRequest(
      connection.get(), method.c_str(), path.c_str(), nullptr,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
      secure ? WINHTTP_FLAG_SECURE : 0)};
  if (!request_handle) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to create the HTTP request"};
  }

  if (this->follow_redirects_) {
    DWORD maximum_redirects{static_cast<DWORD>(this->maximum_redirects_)};
    WinHttpSetOption(request_handle.get(),
                     WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS,
                     &maximum_redirects, sizeof(maximum_redirects));
  } else {
    DWORD policy{WINHTTP_OPTION_REDIRECT_POLICY_NEVER};
    WinHttpSetOption(request_handle.get(), WINHTTP_OPTION_REDIRECT_POLICY,
                     &policy, sizeof(policy));
  }

  // The total timeout bounds sending the request and receiving the response,
  // and also caps the resolution and connection phases unless a narrower
  // connect timeout is given, so it acts as an overall ceiling
  const auto total_timeout{to_winhttp_timeout(this->timeout_)};
  const auto connect_timeout{
      this->connect_timeout_.has_value()
          ? to_winhttp_timeout(this->connect_timeout_.value())
          : total_timeout};
  WinHttpSetTimeouts(request_handle.get(), connect_timeout, connect_timeout,
                     total_timeout, total_timeout);

  DWORD decompression{WINHTTP_DECOMPRESSION_FLAG_ALL};
  WinHttpSetOption(request_handle.get(), WINHTTP_OPTION_DECOMPRESSION,
                   &decompression, sizeof(decompression));

  auto serialized_headers{http_serialize_headers(this->headers_)};
  LPVOID body_data{WINHTTP_NO_REQUEST_DATA};
  DWORD body_size{0};
  if (this->body_.has_value()) {
    if (this->body_.value().data.size() > std::numeric_limits<DWORD>::max()) {
      throw HTTPError{this->method_, this->url_,
                      "The request body is too large"};
    }

    serialized_headers += "Content-Type: ";
    serialized_headers += this->body_.value().content_type;
    serialized_headers += "\r\n";
    body_data = const_cast<char *>(this->body_.value().data.data());
    body_size = static_cast<DWORD>(this->body_.value().data.size());
  }

  const auto request_headers{
      sourcemeta::core::utf8_to_wide(serialized_headers)};

  if (!WinHttpSendRequest(
          request_handle.get(),
          request_headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS
                                  : request_headers.c_str(),
          request_headers.empty() ? 0
                                  : static_cast<DWORD>(request_headers.size()),
          body_data, body_size, body_size, 0)) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to send the HTTP request"};
  }

  if (!WinHttpReceiveResponse(request_handle.get(), nullptr)) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to receive the HTTP response"};
  }

  DWORD status_code{0};
  DWORD status_code_size{sizeof(status_code)};
  if (!WinHttpQueryHeaders(request_handle.get(),
                           WINHTTP_QUERY_STATUS_CODE |
                               WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &status_code,
                           &status_code_size, WINHTTP_NO_HEADER_INDEX)) {
    throw HTTPError{this->method_, this->url_,
                    "Failed to read the HTTP response status"};
  }

  parse_response_headers(request_handle.get(), response.headers);
  response.url = query_effective_url(request_handle.get());

  while (true) {
    DWORD available{0};
    if (!WinHttpQueryDataAvailable(request_handle.get(), &available)) {
      throw HTTPError{this->method_, this->url_,
                      "Failed to read the HTTP response body"};
    }

    if (available == 0) {
      break;
    }

    if (this->maximum_response_size_.has_value() &&
        (response.body.size() > this->maximum_response_size_.value() ||
         available >
             this->maximum_response_size_.value() - response.body.size())) {
      throw HTTPError{this->method_, this->url_,
                      std::string{HTTP_RESPONSE_TOO_LARGE_MESSAGE}};
    }

    const auto offset{response.body.size()};
    response.body.resize(offset + available);
    DWORD read{0};
    if (!WinHttpReadData(request_handle.get(), response.body.data() + offset,
                         available, &read)) {
      throw HTTPError{this->method_, this->url_,
                      "Failed to read the HTTP response body"};
    }

    response.body.resize(offset + read);
  }

  response.status =
      http_status_from_code(static_cast<std::uint16_t>(status_code));
  return response;
}

} // namespace sourcemeta::core
