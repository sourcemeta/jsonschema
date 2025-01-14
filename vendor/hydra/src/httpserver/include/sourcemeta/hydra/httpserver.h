#ifndef SOURCEMETA_HYDRA_HTTPSERVER_H
#define SOURCEMETA_HYDRA_HTTPSERVER_H

#ifndef SOURCEMETA_HYDRA_HTTPSERVER_EXPORT
#include <sourcemeta/hydra/httpserver_export.h>
#endif

/// @defgroup httpserver HTTP Server
/// @brief An server implementation of the HTTP/1.1 protocol
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/hydra/httpserver.h>
/// ```

#include <sourcemeta/hydra/http.h>
#include <sourcemeta/hydra/httpserver_logger.h>
#include <sourcemeta/hydra/httpserver_request.h>
#include <sourcemeta/hydra/httpserver_response.h>

#include <cstdint>    // std::uint32_t
#include <exception>  // std::exception_ptr
#include <filesystem> // std::filesystem::path
#include <functional> // std::function
#include <string>     // std::string
#include <tuple>      // std::tuple
#include <vector>     // std::vector

namespace sourcemeta::hydra::http {

/// @ingroup httpserver
/// This class represents an HTTP server
class SOURCEMETA_HYDRA_HTTPSERVER_EXPORT Server {
public:
  /// Create a server instance. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// // Once the server instance is in place:
  /// // (1) Register your routes
  /// // (2) Bind to the desired port to start accepting incoming connections
  /// ```
  Server();

  /// Represents a callback to an HTTP route
  using RouteCallback = std::function<void(
      const ServerLogger &, const ServerRequest &, ServerResponse &)>;

  /// Represents a callback that handles an exception thrown while processing
  /// another route
  using ErrorCallback =
      std::function<void(std::exception_ptr, const ServerLogger &,
                         const ServerRequest &, ServerResponse &)>;

  /// Register a route to respond to HTTP requests. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_hello_world(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   response.end("Hello World");
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET,
  ///   "/", on_hello_world);
  /// server.route(sourcemeta::hydra::http::Method::PUT,
  ///   "/foo", on_hello_world);
  ///
  /// server.run(3000);
  /// ```
  auto route(const Method method, std::string &&path, RouteCallback &&callback)
      -> void;

  /// Set a handler that responds to HTTP requests that do not match any other
  /// registered routes. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_hello_world(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   response.end("Hello World");
  /// }
  ///
  /// static auto
  /// on_otherwise(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::NOT_FOUND);
  ///   response.end();
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET,
  ///   "/hello", on_hello_world);
  /// server.otherwise(on_otherwise);
  ///
  /// server.run(3000);
  /// ```
  ///
  /// If you don't make use of this method in your server, a default
  /// implementation will be used.
  auto otherwise(RouteCallback &&callback) -> void;

  /// Catch an exception that was thrown while handling any HTTP request to
  /// gracefully respond to the client. To ensure you get access to the original
  /// exception, this callback takes an `std::exception_ptr` that you can
  /// re-throw and handle as you wish. The exception pointer is guaranteed to
  /// not be null. To avoid undefined behavior, your error handler must be
  /// `noexcept`. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  /// #include <stdexcept>
  /// #include <exception>
  ///
  /// sourcemeta::hydra::http::Server server;
  ///
  /// static auto
  /// on_request(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   throw std::runtime_error("Something bad happened!");
  /// }
  ///
  /// static auto
  /// on_error(std::exception_ptr error,
  ///   const sourcemeta::hydra::http::ServerLogger &,
  ///   const sourcemeta::hydra::http::ServerRequest &,
  ///   sourcemeta::hydra::http::ServerResponse &response) noexcept -> void {
  ///   response.status(sourcemeta::hydra::http::Status::INTERNAL_SERVER_ERROR);
  ///
  ///   try {
  ///     std::rethrow_exception(error);
  ///   } catch (const std::runtime_error &exception) {
  ///     response.end(exception.what());
  ///   } catch (const std::exception &exception) {
  ///     response.end(exception.what());
  ///   }
  /// }
  ///
  /// server.route(sourcemeta::hydra::http::Method::GET, "/", on_request);
  /// server.error(on_error);
  ///
  /// server.run(3000);
  /// ```
  ///
  /// If you don't make use of this method in your server, a default
  /// implementation will be used.
  auto error(ErrorCallback &&callback) -> void;

  /// Start the server listening at the desired port. This class will
  /// automatically run the server on a thread pool determined by your hardware
  /// characteristics. This method will only return when an error occurred and
  /// the server could not bind to the desired port. Also, this method returns
  /// an `int` so that it can be convenient used from the `main` entrypoint
  /// function of the program. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/httpserver.h>
  ///
  /// static auto
  /// on_hello_world(const sourcemeta::hydra::http::ServerLogger &,
  ///         const sourcemeta::hydra::http::ServerRequest &,
  ///         sourcemeta::hydra::http::ServerResponse &response) -> void {
  ///   response.status(sourcemeta::hydra::http::Status::OK);
  ///   response.end("Hello World");
  /// }
  ///
  /// int main() {
  ///   sourcemeta::hydra::http::Server server;
  ///   server.route(sourcemeta::hydra::http::Method::GET,
  ///     "/hello", on_hello_world);
  ///   return server.run(3000);
  /// }
  /// ```
  auto run(const std::uint32_t port) const -> int;

private:
  std::vector<std::tuple<Method, std::string, RouteCallback>> routes;
  RouteCallback fallback;
  ErrorCallback error_handler;
  ServerLogger logger{"global"};
};

/// @ingroup httpserver
/// Serve a static file. This function assumes that the file exists and that is
/// not a directory. For example:
///
/// ```cpp
/// #include <sourcemeta/hydra/httpserver.h>
///
/// static auto
/// on_static(const sourcemeta::hydra::http::ServerLogger &,
///         const sourcemeta::hydra::http::ServerRequest &request,
///         sourcemeta::hydra::http::ServerResponse &response) -> void {
///   const std::filesystem::path file_path{"path/to/static" + request.path()};
///   if (!std::filesystem::exists(file_path)) {
///     response.status(sourcemeta::hydra::http::Status::NOT_FOUND);
///     response.end();
///     return;
///   }
///
///   sourcemeta::hydra::http::serve_file(file_path, request, response);
/// }
///
/// int main() {
///   sourcemeta::hydra::http::Server server;
///   server.route(sourcemeta::hydra::http::Method::GET, "/*", on_static);
///   return server.run(3000);
/// }
/// ```
auto SOURCEMETA_HYDRA_HTTPSERVER_EXPORT
serve_file(const std::filesystem::path &file_path, const ServerRequest &request,
           ServerResponse &response, const Status code = Status::OK) -> void;

} // namespace sourcemeta::hydra::http

#endif
