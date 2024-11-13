#ifndef SOURCEMETA_HYDRA_BUCKET_CACHE_POLICY_H
#define SOURCEMETA_HYDRA_BUCKET_CACHE_POLICY_H

namespace sourcemeta::hydra {

/// @ingroup bucket
/// Determines the caching approach in the bucket
enum class BucketCachePolicy {

  /// Assume the contents of the bucket are immutable and cache for as long as
  /// possible
  Indefinitely,

  /// Cache based on the ETag response header of the bucket element
  ETag

};

} // namespace sourcemeta::hydra

#endif
