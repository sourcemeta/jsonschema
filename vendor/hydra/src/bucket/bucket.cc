#include <sourcemeta/hydra/bucket.h>
#include <sourcemeta/hydra/bucket_aws_sigv4.h>
#include <sourcemeta/hydra/crypto.h>
#include <sourcemeta/hydra/httpclient.h>

#include <sourcemeta/core/uri.h>

#include <cassert> // assert
#include <chrono>  // std::chrono::system_clock
#include <sstream> // std::ostringstream
#include <utility> // std::move

namespace sourcemeta::hydra {

Bucket::Bucket(std::string bucket_url, std::string bucket_region,
               std::string bucket_access_key, std::string bucket_secret_key,
               const BucketCachePolicy bucket_cache_policy,
               const std::uint64_t cache_byte_limit)
    // clang-format off
    : cache{std::move(cache_byte_limit)},
      cache_policy{std::move(bucket_cache_policy)},
      url{std::move(bucket_url)},
      region{std::move(bucket_region)},
      access_key{std::move(bucket_access_key)},
      secret_key{std::move(bucket_secret_key)} {}
// clang-format on

auto Bucket::fetch_json(const std::string &key)
    -> std::future<std::optional<ResponseJSON>> {
  std::promise<std::optional<ResponseJSON>> promise;
  assert(key.front() == '/');

  const auto cached_result{this->cache.at(key)};
  if (cached_result.has_value() &&
      this->cache_policy == BucketCachePolicy::Indefinitely) {
    promise.set_value(cached_result.value());
    return promise.get_future();
  }

  // TODO: Properly build, concat, and canonicalize the string using URI Kit
  std::ostringstream request_url;
  request_url << this->url;
  request_url << key;

  sourcemeta::hydra::http::ClientRequest request{request_url.str()};
  request.method(sourcemeta::hydra::http::Method::GET);
  request.capture({"content-type", "etag", "last-modified"});

  if (cached_result.has_value() &&
      this->cache_policy == BucketCachePolicy::ETag) {
    request.header("If-None-Match", cached_result.value().etag);
  }

  // The empty content SHA256
  static constexpr auto empty_content_checksum{
      "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"};

  for (auto &&[header, value] :
       aws_sigv4(request.method(),
                 // TODO: Support constructing a URL given a string_view
                 sourcemeta::core::URI{std::string{request.url()}},
                 this->access_key, this->secret_key, this->region,
                 empty_content_checksum, std::chrono::system_clock::now())) {
    request.header(std::move(header), std::move(value));
  }

  sourcemeta::hydra::http::ClientResponse response{request.send().get()};

  const auto status{response.status()};
  if (status == sourcemeta::hydra::http::Status::NOT_FOUND) {
    promise.set_value(std::nullopt);
    return promise.get_future();
  } else if (status == sourcemeta::hydra::http::Status::NOT_MODIFIED &&
             cached_result.has_value() &&
             this->cache_policy == BucketCachePolicy::ETag) {
    promise.set_value(cached_result.value());
    return promise.get_future();
  } else if (status != sourcemeta::hydra::http::Status::OK) {
    throw BucketError("Failed to fetch JSON from storage");
  }

  assert(!response.empty());
  // No S3-compatible bucket lacks these
  assert(response.header("etag").has_value());
  assert(response.header("last-modified").has_value());

  ResponseJSON result = {sourcemeta::core::parse(response.body()),
                         response.header("etag").value(),
                         response.header_gmt("last-modified").value(), false};

  this->cache.upsert(key,
                     {result.data, result.etag, result.last_modified, true},
                     result.data.estimated_byte_size());

  promise.set_value(std::move(result));
  return promise.get_future();
}

auto Bucket::upsert_json(const std::string &key,
                         const sourcemeta::core::JSON &document)
    -> std::future<void> {
  std::promise<void> promise;
  assert(key.front() == '/');

  // TODO: Properly build, concat, and canonicalize the string using URI Kit
  std::ostringstream request_url;
  request_url << this->url;
  request_url << key;

  sourcemeta::hydra::http::ClientRequest request{request_url.str()};
  request.method(sourcemeta::hydra::http::Method::PUT);
  request.header("content-type", "application/json");

  // TODO: Support chunked streaming uploads instead
  // See https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-streaming.html
  std::stringstream content;
  sourcemeta::core::prettify(document, content);
  std::ostringstream content_checksum;
  sourcemeta::hydra::sha256(content.str(), content_checksum);
  request.header("content-length", std::to_string(content.str().size()));
  request.header("transfer-encoding", "");

  for (auto &&[header, value] :
       aws_sigv4(request.method(),
                 // TODO: Support constructing a URL given a string_view
                 sourcemeta::core::URI{std::string{request.url()}},
                 this->access_key, this->secret_key, this->region,
                 content_checksum.str(), std::chrono::system_clock::now())) {
    request.header(std::move(header), std::move(value));
  }

  sourcemeta::hydra::http::ClientResponse response{request.send(content).get()};
  const auto status{response.status()};
  if (status != sourcemeta::hydra::http::Status::OK) {
    std::ostringstream error;
    error << "Failed to upsert JSON to storage: " << status;
    throw BucketError(error.str());
  }

  return promise.get_future();
}

auto Bucket::fetch_or_upsert(const std::string &key,
                             std::function<sourcemeta::core::JSON()> callback)
    -> std::future<ResponseJSON> {
  std::promise<ResponseJSON> promise;
  auto maybe_response{this->fetch_json(key).get()};
  if (maybe_response.has_value()) {
    promise.set_value(std::move(maybe_response).value());
  } else {
    this->upsert_json(key, callback());
    auto response{this->fetch_json(key).get()};
    assert(response.has_value());
    promise.set_value(std::move(response).value());
  }

  return promise.get_future();
}

} // namespace sourcemeta::hydra
