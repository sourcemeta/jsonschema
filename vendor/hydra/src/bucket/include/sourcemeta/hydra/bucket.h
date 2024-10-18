#ifndef SOURCEMETA_HYDRA_BUCKET_H
#define SOURCEMETA_HYDRA_BUCKET_H

#ifndef SOURCEMETA_HYDRA_BUCKET_EXPORT
#include <sourcemeta/hydra/bucket_export.h>
#endif

/// @defgroup bucket Bucket
/// @brief This module manages interactions with S3-compatible storage modules
/// such as S3 itself, Backblaze B2, MinIO, and probably others.
///
/// This functionality is included as follows:
///
/// ```cpp
/// #include <sourcemeta/hydra/bucket.h>
/// ```

#include <sourcemeta/hydra/bucket_cache.h>
#include <sourcemeta/hydra/bucket_cache_policy.h>
#include <sourcemeta/hydra/bucket_error.h>
#include <sourcemeta/hydra/bucket_response.h>

#include <sourcemeta/jsontoolkit/json.h>

#include <cstdint>    // std::uint64_t
#include <functional> // std::function
#include <future>     // std::future
#include <optional>   // std::optional
#include <string>     // std::string

namespace sourcemeta::hydra {

/// @ingroup bucket
/// This class is used to interact with S3-compatible HTTP buckets. Keep in mind
/// that this class is not thread-safe by default. If you are using it in a
/// concurrent context, make use of `std::mutex` or similar techniques to
/// control access to it.
class SOURCEMETA_HYDRA_BUCKET_EXPORT Bucket {
public:
  /// Create an instance of this class. This class supports automatically
  /// caching responses in an LRU fashion based on a byte-size limit. If you set
  /// the cache byte limit to 0, then caching is effectively disabled.
  ///
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/bucket.h>
  ///
  /// // With caching
  /// sourcemeta::hydra::Bucket bucket_with_cache{
  ///   // A Backblaze B2 bucket
  ///   "https://s3.us-east-005.backblazeb2.com/my-bucket",
  ///   "us-east-005", "123456789", "ultra-secret",
  ///   // Treat the bucket as immutable and perform aggressive caching
  ///   sourcemeta::hydra::BucketCachePolicy::Indefinitely,
  ///   // Roughly 5 MB
  ///   5000000};
  ///
  /// // Without caching
  /// sourcemeta::hydra::Bucket bucket_with_cache{
  ///   // A Backblaze B2 bucket
  ///   "https://s3.us-east-005.backblazeb2.com/my-bucket",
  ///   "us-east-005", "123456789", "ultra-secret"};
  /// ```
  Bucket(std::string url, std::string region, std::string access_key,
         std::string secret_key,
         const BucketCachePolicy cache_policy = BucketCachePolicy::ETag,
         const std::uint64_t cache_byte_limit = 0);

  // While technically possible, copy semantics are not very useful here
  // Also, not worth documenting these.
#if !defined(DOXYGEN)
  Bucket(const Bucket &other) = delete;
  auto operator=(const Bucket &other) -> Bucket & = delete;
#endif

  /// Represents a JSON response from a bucket
  using ResponseJSON = BucketResponse<sourcemeta::jsontoolkit::JSON>;

  /// Fetch a JSON document from the given bucket. The key must start with a
  /// forward slash. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/bucket.h>
  /// #include <sourcemeta/jsontoolkit/json.h>
  ///
  /// #include <iostream>
  ///
  /// sourcemeta::hydra::Bucket bucket{
  ///   // A Backblaze B2 bucket
  ///   "https://s3.us-east-005.backblazeb2.com/my-bucket",
  ///   "us-east-005", "123456789", "ultra-secret",
  ///   // Treat the bucket as immutable and perform aggressive caching
  ///   sourcemeta::hydra::BucketCachePolicy::Indefinitely,
  ///   // Roughly 5 MB
  ///   5000000};
  ///
  /// std::optional<sourcemeta::hydra::Bucket::ResponseJSON> response{
  ///   bucket.fetch_json("/foo/bar.json").get()};
  ///
  /// if (response.has_value()) {
  ///   sourcemeta::jsontoolkit::prettify(response.value().data, std::cout);
  ///   std::cout << "\n";
  ///   std::cout << "ETag: " << response.value().etag << "\n";
  ///   std::cout << "Last-Modified: "
  ///     << response.value().last_modified << "\n";
  ///
  ///   if (response.value().cache_hit) {
  ///     std::cout << "This was a cache hit!\n";
  ///   }
  /// }
  /// ```
  ///
  /// Keep in mind this function does not perform any normalization to the key,
  /// so to avoid unexpected results (mainly around caching), make sure to pass
  /// keys in a consistent manner (i.e. casing, etc).
  auto fetch_json(const std::string &key)
      -> std::future<std::optional<ResponseJSON>>;

  /// Upsert a JSON document into the given bucket. The key must start with a
  /// forward slash. For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/bucket.h>
  /// #include <sourcemeta/jsontoolkit/json.h>
  ///
  /// sourcemeta::hydra::Bucket bucket{
  ///   // A Backblaze B2 bucket
  ///   "https://s3.us-east-005.backblazeb2.com/my-bucket",
  ///   "us-east-005", "123456789", "ultra-secret"};
  ///
  /// const sourcemeta::jsontoolkit::JSON document =
  ///   sourcemeta::jsontoolkit::parse("{ \"foo\": \"bar\" }");
  ///
  /// bucket.upsert_json("/foo/bar.json", document).wait();
  /// ```
  auto upsert_json(const std::string &key,
                   const sourcemeta::jsontoolkit::JSON &document)
      -> std::future<void>;

  /// Upsert a JSON document into the given bucket unless it already exists. If
  /// so, just return the existing one. The key must start with a forward slash.
  /// For example:
  ///
  /// ```cpp
  /// #include <sourcemeta/hydra/bucket.h>
  /// #include <sourcemeta/jsontoolkit/json.h>
  ///
  /// sourcemeta::hydra::Bucket bucket{
  ///   // A Backblaze B2 bucket
  ///   "https://s3.us-east-005.backblazeb2.com/my-bucket",
  ///   "us-east-005", "123456789", "ultra-secret"};
  ///
  /// std::optional<sourcemeta::hydra::Bucket::ResponseJSON> response{
  ///   bucket.fetch_or_upsert("/foo/bar.json",
  ///     []() -> sourcemeta::jsontoolkit::JSON {
  ///       return sourcemeta::jsontoolkit::parse("{ \"foo\": \"bar\" }");
  ///     }).get()};
  ///
  /// if (response.has_value()) {
  ///   sourcemeta::jsontoolkit::prettify(response.value().data, std::cout);
  ///   std::cout << "\n";
  ///   std::cout << "ETag: " << response.value().etag << "\n";
  ///   std::cout << "Last-Modified: "
  ///     << response.value().last_modified << "\n";
  ///
  ///   if (response.value().cache_hit) {
  ///     std::cout << "This was a cache hit!\n";
  ///   }
  /// }
  /// ```
  auto fetch_or_upsert(const std::string &key,
                       std::function<sourcemeta::jsontoolkit::JSON()> callback)
      -> std::future<ResponseJSON>;

private:
#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
  BucketCache<ResponseJSON> cache;
  const BucketCachePolicy cache_policy;
  const std::string url;
  const std::string region;
  const std::string access_key;
  const std::string secret_key;
#if defined(_MSC_VER)
#pragma warning(default : 4251)
#endif
};

} // namespace sourcemeta::hydra

#endif
