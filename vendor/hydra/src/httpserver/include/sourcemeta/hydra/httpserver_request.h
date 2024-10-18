#ifndef SOURCEMETA_HYDRA_HTTPSERVER_REQUEST_H
#define SOURCEMETA_HYDRA_HTTPSERVER_REQUEST_H

#ifndef SOURCEMETA_HYDRA_HTTPSERVER_EXPORT
#include <sourcemeta/hydra/httpserver_export.h>
#endif

#include <sourcemeta/hydra/http.h>

#include <chrono>      // std::chrono::system_clock::time_point
#include <cstdint>     // std::uint8_t
#include <memory>      // std::unique_ptr
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::hydra::http {

/// @ingroup httpserver
/// This class encapsulates the incoming HTTP request
class SOURCEMETA_HYDRA_HTTPSERVER_EXPORT ServerRequest {
public:
// These constructors are considered private. Do not use them directly.
#if !defined(DOXYGEN)
  ServerRequest(void *const handler);
  ~ServerRequest();
#endif

  /// Get the HTTP method of the incoming request. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   std::ostringstream result;
  ///   result << "Got method: " << request.method();
  ///   response.end(result.str());
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto method() const -> Method;

  /// Get the value of a header of the incoming request. The header name is
  /// expected to be lowercase. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   const auto host{request.header("host")};
  ///   assert(host.has_value());
  ///   std::ostringstream result;
  ///   result << "The host is: " << host.value();
  ///   response.end(result.str());
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto header(std::string_view key) const -> std::optional<std::string>;

  /// Get the value of a header of the incoming request assuming it consists of
  /// comma-separated elements. The header name is expected to be lowercase. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &logger,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   const auto encodings{request.header_list("accept-encoding")};
  ///   if (encodings.has_value()) {
  ///     for (const auto &encoding : encodings.value()) {
  ///       logger << encoding.first;
  ///     }
  ///   }
  ///
  ///   response.end();
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto header_list(std::string_view key) const
      -> std::optional<std::vector<HeaderListElement>>;

  /// Get the value of a header of the incoming request assuming it consists of
  /// a GMT timestamp. The header name is expected to be lowercase. For
  /// example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &logger,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   const auto if_modified_since{request.header_list("if-modified-since")};
  ///   if (if_modified_since.has_value()) {
  ///     logger <<
  ///       sourcemeta::hydra::http::from_gmt(if_modified_since.value());
  ///   }
  ///
  ///   response.end();
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto header_gmt(std::string_view key) const
      -> std::optional<std::chrono::system_clock::time_point>;

  /// Evaluate the timestamp passed using `If-Modified-Since` by the client, if
  /// any, against the resource last modification timestamp. If so, you should
  /// respond with `304 Not Modified`. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   const auto timestamp{
  ///     sourcemeta::hydra::http::from_gmt("Wed, 21 Oct 2015 11:28:00 GMT")};
  ///
  ///   if (!request.header_if_modified_since(timestamp)) {
  ///     response.status(sourcemeta::hydra::http::Status::NOT_MODIFIED);
  ///     response.end();
  ///     return;
  ///   }
  ///
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   response.header_last_modified(timestamp);
  ///   response.end("Foo Bar");
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto header_if_modified_since(
      const std::chrono::system_clock::time_point last_modified) const -> bool;

  /// Evaluate the values passed using `If-None-Match` by the client, if
  /// any, against the resource `ETag` value. If so, you should respond with
  /// `304 Not Modified`. Keep in mind that you shouldn't pass a weak
  /// `ETag` value to this function, but the value directly (with or without
  /// quotes). For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   const auto checksum{"711d2f4adab4515e4036c48bf58eb975"};
  ///
  ///   if (!request.header_if_none_match(checksum)) {
  ///     response.status(sourcemeta::hydra::http::Status::NOT_MODIFIED);
  ///     response.end();
  ///     return;
  ///   }
  ///
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   response.header_etag(checksum);
  ///   response.end("Foo Bar");
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto header_if_none_match(std::string_view etag) const -> bool;

  /// Get the value of a query string in the incoming request URL. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   std::ostringstream result;
  ///   const auto foo{request.query("foo")};
  ///   if (foo.has_value()) {
  ///     result << "Foo: " << foo.value();
  ///   } else {
  ///     result << "Try passing a ?foo= query string\n";
  ///   }
  ///
  ///   response.end(result.str());
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto query(std::string_view key) const -> std::optional<std::string>;

  /// Get the path of the incoming request URL. Note that the path does not
  /// include query or fragments. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   std::ostringstream result;
  ///   result << request.path();
  ///   response.end(result.str());
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_root);
  /// ```
  auto path() const -> std::string;

  /// Get a parameter of the request URL by position. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <sstream>
  /// #include <cassert>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_root(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &request,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   std::ostringstream result;
  ///   // Matches ":bar"
  ///   result << request.parameter(0);
  ///   response.end(result.str());
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/foo/:bar", on_root);
  /// ```
  auto parameter(const std::uint8_t index) const -> std::string;

private:
  // PIMPL idiom to hide uWebSockets
  struct Internal;
  std::unique_ptr<Internal> internal;
};

} // namespace sourcemeta::hydra::http

#endif
