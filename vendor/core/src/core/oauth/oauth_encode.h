#ifndef SOURCEMETA_CORE_OAUTH_ENCODE_H_
#define SOURCEMETA_CORE_OAUTH_ENCODE_H_

#include <sourcemeta/core/uri.h>

#include <string_view> // std::string_view

namespace sourcemeta::core {

// Append one "application/x-www-form-urlencoded" parameter to a body under
// construction, inserting the "&" separator only between parameters so that
// grant and client authentication builders compose into one buffer. The name
// and value are percent-encoded through the URI escaper into the sink, which is
// a wiping string for a body that carries secrets and an ordinary string
// otherwise
template <typename Sink>
inline auto oauth_append_form_parameter(Sink &sink, const std::string_view name,
                                        const std::string_view value) -> void {
  if (!sink.empty() && sink.back() != '&') {
    sink.push_back('&');
  }

  URI::escape(name, sink);
  sink.push_back('=');
  URI::escape(value, sink);
}

} // namespace sourcemeta::core

#endif
