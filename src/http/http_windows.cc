#include "http.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h> // MultiByteToWideChar, WideCharToMultiByte, DWORD
#include <winhttp.h> // WinHttp*

#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair
#include <vector>      // std::vector

namespace {

auto to_wide(const std::string_view input) -> std::wstring {
  if (input.empty()) {
    return L"";
  }

  const auto size{MultiByteToWideChar(
      CP_UTF8, 0, input.data(), static_cast<int>(input.size()), nullptr, 0)};
  std::wstring result(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
                      result.data(), size);
  return result;
}

auto to_narrow(const std::wstring &input) -> std::string {
  if (input.empty()) {
    return "";
  }

  const auto size{WideCharToMultiByte(CP_UTF8, 0, input.c_str(),
                                      static_cast<int>(input.size()), nullptr,
                                      0, nullptr, nullptr)};
  std::string result(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()),
                      result.data(), size, nullptr, nullptr);
  return result;
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

  sourcemeta::jsonschema::http_parse_headers(to_narrow(buffer), headers);
}

} // namespace

namespace sourcemeta::jsonschema {

auto http_request(const HTTPRequest &request) -> HTTPResponse {
  HTTPResponse response;

  const auto wide_url{to_wide(request.url)};
  URL_COMPONENTS components{};
  components.dwStructSize = sizeof(components);
  components.dwHostNameLength = static_cast<DWORD>(-1);
  components.dwUrlPathLength = static_cast<DWORD>(-1);
  components.dwExtraInfoLength = static_cast<DWORD>(-1);
  if (!WinHttpCrackUrl(wide_url.c_str(), 0, 0, &components)) {
    throw HTTPError{request.method, std::string{request.url}, "Invalid URL"};
  }

  const std::wstring host{components.lpszHostName, components.dwHostNameLength};
  std::wstring path{components.lpszUrlPath, components.dwUrlPathLength};
  if (components.lpszExtraInfo) {
    path.append(components.lpszExtraInfo, components.dwExtraInfoLength);
  }

  const WinHTTPHandle session{
      WinHttpOpen(nullptr, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
  if (!session) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to initialise the HTTP client"};
  }

  const WinHTTPHandle connection{
      WinHttpConnect(session.get(), host.c_str(), components.nPort, 0)};
  if (!connection) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to connect to the host"};
  }

  const auto secure{components.nScheme == INTERNET_SCHEME_HTTPS};
  const auto method{to_wide(http_method_string(request.method))};
  const WinHTTPHandle request_handle{WinHttpOpenRequest(
      connection.get(), method.c_str(), path.c_str(), nullptr,
      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
      secure ? WINHTTP_FLAG_SECURE : 0)};
  if (!request_handle) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to create the HTTP request"};
  }

  DWORD decompression{WINHTTP_DECOMPRESSION_FLAG_ALL};
  WinHttpSetOption(request_handle.get(), WINHTTP_OPTION_DECOMPRESSION,
                   &decompression, sizeof(decompression));

  auto serialized_headers{http_serialize_headers(request.headers)};
  LPVOID body_data{WINHTTP_NO_REQUEST_DATA};
  DWORD body_size{0};
  if (request.body.has_value()) {
    serialized_headers += "Content-Type: ";
    serialized_headers += request.body.value().content_type;
    serialized_headers += "\r\n";
    body_data = const_cast<char *>(request.body.value().data.data());
    body_size = static_cast<DWORD>(request.body.value().data.size());
  }

  const auto request_headers{to_wide(serialized_headers)};

  if (!WinHttpSendRequest(
          request_handle.get(),
          request_headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS
                                  : request_headers.c_str(),
          request_headers.empty() ? 0
                                  : static_cast<DWORD>(request_headers.size()),
          body_data, body_size, body_size, 0)) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to send the HTTP request"};
  }

  if (!WinHttpReceiveResponse(request_handle.get(), nullptr)) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to receive the HTTP response"};
  }

  DWORD status_code{0};
  DWORD status_code_size{sizeof(status_code)};
  if (!WinHttpQueryHeaders(request_handle.get(),
                           WINHTTP_QUERY_STATUS_CODE |
                               WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &status_code,
                           &status_code_size, WINHTTP_NO_HEADER_INDEX)) {
    throw HTTPError{request.method, std::string{request.url},
                    "Failed to read the HTTP response status"};
  }

  parse_response_headers(request_handle.get(), response.headers);

  while (true) {
    DWORD available{0};
    if (!WinHttpQueryDataAvailable(request_handle.get(), &available)) {
      throw HTTPError{request.method, std::string{request.url},
                      "Failed to read the HTTP response body"};
    }

    if (available == 0) {
      break;
    }

    if (request.maximum_response_size.has_value() &&
        response.body.size() + available >
            request.maximum_response_size.value()) {
      throw HTTPError{request.method, std::string{request.url},
                      std::string{HTTP_RESPONSE_TOO_LARGE_MESSAGE}};
    }

    const auto offset{response.body.size()};
    response.body.resize(offset + available);
    DWORD read{0};
    if (!WinHttpReadData(request_handle.get(), response.body.data() + offset,
                         available, &read)) {
      throw HTTPError{request.method, std::string{request.url},
                      "Failed to read the HTTP response body"};
    }

    response.body.resize(offset + read);
  }

  response.status =
      http_status_from_code(static_cast<std::uint16_t>(status_code));
  return response;
}

} // namespace sourcemeta::jsonschema
