#include <sourcemeta/hydra/bucket_aws_sigv4.h>
#include <sourcemeta/hydra/crypto.h>

#include <cassert>   // assert
#include <ctime>     // std::time_t, std::tm, std::gmtime
#include <iomanip>   // std::setfill, std::setw, std::put_time
#include <ios>       // std::hex
#include <sstream>   // std::ostringstream
#include <stdexcept> // std::runtime_error
#include <utility>   // std::move

namespace sourcemeta::hydra {

static inline auto
write_date_with_format(const std::chrono::system_clock::time_point time,
                       const char *const format, std::ostream &output) -> void {
  const std::time_t ctime{std::chrono::system_clock::to_time_t(time)};
#if defined(_MSC_VER)
  std::tm buffer;
  if (gmtime_s(&buffer, &ctime) != 0) {
    throw std::runtime_error(
        "Could not convert time point to the desired format");
  }

  const std::tm *const parts = &buffer;
#else
  const std::tm *const parts = std::gmtime(&ctime);
#endif
  assert(parts);
  output << std::put_time(parts, format);
}

auto aws_sigv4_datastamp(const std::chrono::system_clock::time_point time,
                         std::ostream &output) -> void {
  write_date_with_format(time, "%Y%m%d", output);
}

auto aws_sigv4_iso8601(const std::chrono::system_clock::time_point time,
                       std::ostream &output) -> void {
  write_date_with_format(time, "%Y%m%dT%H%M%SZ", output);
}

auto aws_sigv4_scope(std::string_view datastamp, std::string_view region,
                     std::ostream &output) -> void {
  output << datastamp;
  output << '/';
  output << region;
  output << '/';
  output << "s3";
  output << '/';
  output << "aws4_request";
}

auto aws_sigv4_key(std::string_view secret_key, std::string_view region,
                   std::string_view datastamp) -> std::string {
  std::ostringstream hmac_date;
  sourcemeta::hydra::hmac_sha256(std::string{"AWS4"} + std::string{secret_key},
                                 datastamp, hmac_date);
  std::ostringstream hmac_region;
  sourcemeta::hydra::hmac_sha256(hmac_date.str(), region, hmac_region);
  std::ostringstream hmac_service;
  sourcemeta::hydra::hmac_sha256(hmac_region.str(), "s3", hmac_service);
  std::ostringstream signing_key;
  sourcemeta::hydra::hmac_sha256(hmac_service.str(), "aws4_request",
                                 signing_key);
  return signing_key.str();
}

auto aws_sigv4_canonical(const http::Method method, std::string_view host,
                         std::string_view path,
                         std::string_view content_checksum,
                         std::string_view timestamp) -> std::string {
  std::ostringstream canonical;
  canonical << method << '\n';
  canonical << path << '\n';
  // We don't require query parameters
  canonical << '\n';
  canonical << "host:" << host << "\n";
  canonical << "x-amz-content-sha256:" << content_checksum << "\n";
  canonical << "x-amz-date:" << timestamp << "\n";
  canonical << '\n';
  canonical << "host;x-amz-content-sha256;x-amz-date";
  canonical << '\n';
  canonical << content_checksum;
  return canonical.str();
}

auto aws_sigv4(const http::Method method, const sourcemeta::core::URI &url,
               std::string_view access_key, std::string_view secret_key,
               std::string_view region, std::string &&content_checksum,
               const std::chrono::system_clock::time_point now)
    -> std::map<std::string, std::string> {
  std::ostringstream request_date_iso8601;
  aws_sigv4_iso8601(now, request_date_iso8601);
  std::ostringstream request_date_datastamp;
  aws_sigv4_datastamp(now, request_date_datastamp);
  // See https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/host
  std::ostringstream http_host;
  assert(url.host().has_value());
  http_host << url.host().value();
  if (url.port().has_value()) {
    http_host << ':' << url.port().value();
  }

  // Canonical request
  const auto canonical{sourcemeta::hydra::aws_sigv4_canonical(
      method, http_host.str(), url.path().value_or("/"), content_checksum,
      request_date_iso8601.str())};

  // String to sign
  std::ostringstream string_to_sign;
  string_to_sign << "AWS4-HMAC-SHA256\n";
  string_to_sign << request_date_iso8601.str() << '\n';
  aws_sigv4_scope(request_date_datastamp.str(), region, string_to_sign);
  string_to_sign << '\n';
  sourcemeta::hydra::sha256(canonical, string_to_sign);

  // Authorization
  std::ostringstream authorization;
  authorization << "AWS4-HMAC-SHA256";
  authorization << ' ';
  authorization << "Credential=";
  authorization << access_key;
  authorization << '/';
  aws_sigv4_scope(request_date_datastamp.str(), region, authorization);
  authorization << ", ";
  authorization << "SignedHeaders=";
  authorization << "host;x-amz-content-sha256;x-amz-date";
  authorization << ", ";
  authorization << "Signature=";

  const auto signing_key{
      aws_sigv4_key(secret_key, region, request_date_datastamp.str())};
  std::ostringstream signature;
  sourcemeta::hydra::hmac_sha256(signing_key, string_to_sign.str(), signature);
  authorization << std::hex << std::setfill('0');
  for (const auto character : signature.str()) {
    authorization << std::setw(2)
                  << static_cast<unsigned int>(
                         static_cast<unsigned char>(character));
  }

  return {{"host", std::move(http_host).str()},
          {"x-amz-content-sha256", std::move(content_checksum)},
          {"x-amz-date", std::move(request_date_iso8601).str()},
          {"authorization", std::move(authorization).str()}};
}

} // namespace sourcemeta::hydra
