#include "cpr/response.h"
#include <cassert>
#include <cpr/cert_info.h>
#include <cpr/cookies.h>
#include <cpr/cprtypes.h>
#include <cpr/curlholder.h>
#include <cpr/error.h>
#include <cpr/util.h>
#include <curl/curl.h>
#include <curl/curlver.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cpr {

Response::Response(std::shared_ptr<CurlHolder> curl, std::string&& p_text, std::string&& p_header_string, Cookies&& p_cookies = Cookies{}, Error&& p_error = Error{}) : curl_(std::move(curl)), text(std::move(p_text)), cookies(std::move(p_cookies)), error(std::move(p_error)), raw_header(std::move(p_header_string)) {
    header = cpr::util::parseHeader(raw_header, &status_line, &reason);
    assert(curl_);
    assert(curl_->handle);
    curl_easy_getinfo(curl_->handle, CURLINFO_RESPONSE_CODE, &status_code);
    curl_easy_getinfo(curl_->handle, CURLINFO_TOTAL_TIME, &elapsed);
    char* url_string{nullptr};
    curl_easy_getinfo(curl_->handle, CURLINFO_EFFECTIVE_URL, &url_string);
    url = Url(url_string);
#if LIBCURL_VERSION_NUM >= 0x073700 // 7.55.0
    curl_easy_getinfo(curl_->handle, CURLINFO_SIZE_DOWNLOAD_T, &downloaded_bytes);
    curl_easy_getinfo(curl_->handle, CURLINFO_SIZE_UPLOAD_T, &uploaded_bytes);
#else
    double downloaded_bytes_double, uploaded_bytes_double;
    curl_easy_getinfo(curl_->handle, CURLINFO_SIZE_DOWNLOAD, &downloaded_bytes_double);
    curl_easy_getinfo(curl_->handle, CURLINFO_SIZE_UPLOAD, &uploaded_bytes_double);
    downloaded_bytes = downloaded_bytes_double;
    uploaded_bytes = uploaded_bytes_double;
#endif
    curl_easy_getinfo(curl_->handle, CURLINFO_REDIRECT_COUNT, &redirect_count);
#if LIBCURL_VERSION_NUM >= 0x071300 // 7.19.0
    char* ip_ptr{nullptr};
    if (curl_easy_getinfo(curl_->handle, CURLINFO_PRIMARY_IP, &ip_ptr) == CURLE_OK && ip_ptr) {
        primary_ip = ip_ptr;
    }
#endif
#if LIBCURL_VERSION_NUM >= 0x071500 // 7.21.0
    // Ignored here since libcurl uses a long for this.
    // NOLINTNEXTLINE(google-runtime-int)
    long port = 0;
    if (curl_easy_getinfo(curl_->handle, CURLINFO_PRIMARY_PORT, &port) == CURLE_OK) {
        primary_port = port;
    }
#endif
}

std::vector<CertInfo> Response::GetCertInfos() const {
    assert(curl_);
    assert(curl_->handle);
    curl_certinfo* ci{nullptr};
    curl_easy_getinfo(curl_->handle, CURLINFO_CERTINFO, &ci);

    std::vector<CertInfo> cert_infos;
    for (int i = 0; i < ci->num_of_certs; i++) {
        CertInfo cert_info;
        // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (curl_slist* slist = ci->certinfo[i]; slist; slist = slist->next) {
            cert_info.emplace_back(std::string{slist->data});
        }
        cert_infos.emplace_back(cert_info);
    }
    return cert_infos;
}
} // namespace cpr
