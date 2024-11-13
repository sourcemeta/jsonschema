#ifndef SOURCEMETA_HYDRA_CRYPTO_BEARSSL_H
#define SOURCEMETA_HYDRA_CRYPTO_BEARSSL_H

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#elif defined(_MSC_VER)
#pragma warning(disable : 4244 4267)
#endif
extern "C" {
#include <bearssl.h>
}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(default : 4244 4267)
#endif

#endif
