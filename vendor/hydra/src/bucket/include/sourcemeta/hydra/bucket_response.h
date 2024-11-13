#ifndef SOURCEMETA_HYDRA_BUCKET_RESPONSE_H
#define SOURCEMETA_HYDRA_BUCKET_RESPONSE_H

#include <chrono> // std::chrono::system_clock::time_point
#include <string> // std::string

namespace sourcemeta::hydra {
/// @ingroup bucket
/// This class represents a Bucket response.
template <typename T> struct BucketResponse {
  /// The response data itself
  T data;
  /// The ETag HTTP header that the server responded with
  std::string etag;
  /// The parsed Last-Modified HTTP header that the server responded with
  std::chrono::system_clock::time_point last_modified;
  /// Whether the response resulted in an in-memory cache hit
  bool cache_hit;
};
} // namespace sourcemeta::hydra

#endif
