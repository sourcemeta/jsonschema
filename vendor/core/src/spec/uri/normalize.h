#ifndef SOURCEMETA_CORE_URI_NORMALIZE_H_
#define SOURCEMETA_CORE_URI_NORMALIZE_H_

#include <cassert>     // assert
#include <cstddef>     // std::size_t
#include <string>      // std::string
#include <string_view> // std::string_view

namespace sourcemeta::core {

// Remove "." and ".." segments from a URI path per RFC 3986 Section 5.2.4
// (Remove Dot Segments). For absolute paths this matches the specification
// verbatim. For relative paths, leading "../" blocks are preserved as an
// extension because the spec algorithm assumes a path that has already been
// merged with an absolute base; applied to a stand-alone relative path it
// would discard semantic intent that is needed at later resolution time.
// Pass `resolving` for any path that reference resolution runs through that
// section, where the algorithm applies exactly as written and the extension
// above is not wanted. A leading block then goes rather than being kept, and
// the slash that step C leaves behind stays where the algorithm puts it.
[[nodiscard]] inline auto normalize_path(std::string_view input,
                                         const bool resolving = false)
    -> std::string {
  std::string output;
  output.reserve(input.size());
  const bool is_absolute{!input.empty() && input.front() == '/'};

  while (!input.empty()) {
    if (input.starts_with("../")) {
      if (!resolving) {
        output.append("../");
      }
      input.remove_prefix(3);
    } else if (input.starts_with("./") || input.starts_with("/./")) {
      input.remove_prefix(2);
    } else if (input == "/.") {
      output.push_back('/');
      break;
    } else if (input.starts_with("/../")) {
      input.remove_prefix(3);
      const auto last_slash{output.rfind('/')};
      if (last_slash == std::string::npos) {
        output.clear();
        assert(!input.empty() && input.front() == '/');
        // Step C leaves this slash in the input for step E to move into the
        // result, so only the extension above has reason to drop it
        if (!is_absolute && !resolving) {
          input.remove_prefix(1);
        }
      } else {
        output.resize(last_slash);
      }
    } else if (input == "/..") {
      const auto last_slash{output.rfind('/')};
      if (last_slash == std::string::npos) {
        output.clear();
        if (is_absolute || resolving) {
          output.push_back('/');
        }
      } else {
        output.resize(last_slash);
        output.push_back('/');
      }
      break;
    } else if (input == ".") {
      break;
    } else if (input == "..") {
      assert(!is_absolute);
      if (!resolving) {
        output.append("../");
      }
      break;
    } else {
      const std::size_t next_slash{input.starts_with('/') ? input.find('/', 1)
                                                          : input.find('/')};
      if (next_slash == std::string_view::npos) {
        output.append(input);
        break;
      }
      output.append(input.substr(0, next_slash));
      input.remove_prefix(next_slash);
    }
  }

  return output;
}

// Remove "." and ".." segments from a URI path in place
inline auto normalize_path(std::string &path, const bool resolving = false)
    -> void {
  path = normalize_path(std::string_view{path}, resolving);
}

} // namespace sourcemeta::core

#endif
