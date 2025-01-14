#include <sourcemeta/hydra/crypto.h>
#include <sourcemeta/hydra/httpserver.h>

#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <fstream>    // std::ifstream
#include <sstream>    // std::ostringstream

namespace sourcemeta::hydra::http {

auto serve_file(const std::filesystem::path &file_path,
                const ServerRequest &request, ServerResponse &response,
                const Status code) -> void {
  assert(request.method() == sourcemeta::hydra::http::Method::GET ||
         request.method() == sourcemeta::hydra::http::Method::HEAD);

  // Its the responsibility of the caller to ensure the file path
  // exists, otherwise we cannot know how the application prefers
  // to react to such case.
  assert(std::filesystem::exists(file_path));
  assert(std::filesystem::is_regular_file(file_path));

  const auto last_write_time{std::filesystem::last_write_time(file_path)};
  const auto last_modified{
      std::chrono::time_point_cast<std::chrono::system_clock::duration>(
          last_write_time - std::filesystem::file_time_type::clock::now() +
          std::chrono::system_clock::now())};

  if (!request.header_if_modified_since(last_modified)) {
    response.status(sourcemeta::hydra::http::Status::NOT_MODIFIED);
    response.end();
    return;
  }

  std::ifstream stream{std::filesystem::canonical(file_path)};
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  assert(!stream.fail());
  assert(stream.is_open());

  std::ostringstream contents;
  contents << stream.rdbuf();
  std::ostringstream etag;
  sourcemeta::hydra::md5(contents.str(), etag);

  if (!request.header_if_none_match(etag.str())) {
    response.status(sourcemeta::hydra::http::Status::NOT_MODIFIED);
    response.end();
    return;
  }

  response.status(code);
  response.header("Content-Type", sourcemeta::hydra::mime_type(file_path));
  response.header_etag(etag.str());
  response.header_last_modified(last_modified);

  if (request.method() == sourcemeta::hydra::http::Method::HEAD) {
    response.head(contents.str());
  } else {
    response.end(contents.str());
  }
}

} // namespace sourcemeta::hydra::http
