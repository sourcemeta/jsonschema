#include <sourcemeta/hydra/httpserver_response.h>

#include "uwebsockets.h"

#include <zlib.h>

#include <cassert>     // assert
#include <cstring>     // memset
#include <sstream>     // std::ostringstream
#include <stdexcept>   // std::runtime_error
#include <string>      // std::string
#include <string_view> // std::string_view

static auto zlib_compress_gzip(std::string_view input) -> std::string {
  z_stream stream;
  memset(&stream, 0, sizeof(stream));
  int code = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                          16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
  if (code != Z_OK) {
    throw std::runtime_error("deflateInit2 failed while compressing");
  }

  stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.data()));
  stream.avail_in = static_cast<uInt>(input.size());

  char buffer[4096];
  std::ostringstream compressed;

  do {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);
    code = deflate(&stream, Z_FINISH);
    compressed.write(buffer, sizeof(buffer) - stream.avail_out);
  } while (code == Z_OK);

  if (code != Z_STREAM_END) {
    throw std::runtime_error("Compression failure");
  }

  code = deflateEnd(&stream);
  if (code != Z_OK) {
    throw std::runtime_error("Compression failure");
  }

  return compressed.str();
}

namespace sourcemeta::hydra::http {

struct ServerResponse::Internal {
  uWS::HttpResponse<true> *handler;
};

ServerResponse::ServerResponse(void *const handler)
    : internal{std::make_unique<ServerResponse::Internal>()} {
  assert(handler);
  this->internal->handler = static_cast<uWS::HttpResponse<true> *>(handler);
}

ServerResponse::~ServerResponse() {}

auto ServerResponse::status(const Status status_code) -> void {
  this->code = status_code;
}

auto ServerResponse::status() const -> Status { return this->code; }

auto ServerResponse::header(std::string_view key, std::string_view value)
    -> void {
  this->headers.emplace(key, value);
}

auto ServerResponse::header_last_modified(
    const std::chrono::system_clock::time_point time) -> void {
  this->header("Last-Modified", to_gmt(time));
}

auto ServerResponse::header_etag(std::string_view value) -> void {
  assert(!value.empty());
  assert(!value.starts_with('W'));

  if (value.starts_with('"') && value.ends_with('"')) {
    this->header("ETag", value);
  } else {
    std::ostringstream etag;
    etag << '"' << value << '"';
    this->header("ETag", etag.str());
  }
}

auto ServerResponse::header_etag_weak(std::string_view value) -> void {
  assert(!value.empty());

  if (value.starts_with('W')) {
    this->header("ETag", value);
  } else if (value.starts_with('"') && value.ends_with('"')) {
    std::ostringstream etag;
    etag << 'W' << '/' << value;
    this->header("ETag", etag.str());
  } else {
    std::ostringstream etag;
    etag << 'W' << '/' << '"' << value << '"';
    this->header("ETag", etag.str());
  }
}

auto ServerResponse::encoding(const ServerContentEncoding encoding) -> void {
  switch (encoding) {
    case ServerContentEncoding::GZIP:
      this->header("Content-Encoding", "gzip");
      break;
    case ServerContentEncoding::Identity:
      break;
  }

  this->content_encoding = encoding;
}

auto ServerResponse::end(const std::string_view message) -> void {
  std::ostringstream code_string;
  code_string << this->code;
  this->internal->handler->writeStatus(code_string.str());

  for (const auto &[key, value] : this->headers) {
    this->internal->handler->writeHeader(key, value);
  }

  switch (this->content_encoding) {
    case ServerContentEncoding::GZIP:
      this->internal->handler->end(zlib_compress_gzip(message));

      break;
    case ServerContentEncoding::Identity:
      this->internal->handler->end(message);

      break;
  }
}

auto ServerResponse::head(const std::string_view message) -> void {
  std::ostringstream code_string;
  code_string << this->code;
  this->internal->handler->writeStatus(code_string.str());

  for (const auto &[key, value] : this->headers) {
    this->internal->handler->writeHeader(key, value);
  }

  switch (this->content_encoding) {
    case ServerContentEncoding::GZIP:
      this->internal->handler->endWithoutBody(
          zlib_compress_gzip(message).size());
      this->internal->handler->end();
      break;
    case ServerContentEncoding::Identity:
      this->internal->handler->endWithoutBody(message.size());
      this->internal->handler->end();
      break;
  }
}

auto ServerResponse::end(const sourcemeta::core::JSON &document) -> void {
  std::ostringstream output;
  sourcemeta::core::prettify(document, output);
  this->end(output.str());
}

auto ServerResponse::head(const sourcemeta::core::JSON &document) -> void {
  std::ostringstream output;
  sourcemeta::core::prettify(document, output);
  this->head(output.str());
}

auto ServerResponse::end(const sourcemeta::core::JSON &document,
                         const sourcemeta::core::JSON::KeyComparison &compare)
    -> void {
  std::ostringstream output;
  sourcemeta::core::prettify(document, output, compare);
  this->end(output.str());
}

auto ServerResponse::head(const sourcemeta::core::JSON &document,
                          const sourcemeta::core::JSON::KeyComparison &compare)
    -> void {
  std::ostringstream output;
  sourcemeta::core::prettify(document, output, compare);
  this->head(output.str());
}

auto ServerResponse::end() -> void {
  std::ostringstream code_string;
  code_string << this->code;
  this->internal->handler->writeStatus(code_string.str());

  for (const auto &[key, value] : this->headers) {
    this->internal->handler->writeHeader(key, value);
  }

  this->internal->handler->end();
}

} // namespace sourcemeta::hydra::http
