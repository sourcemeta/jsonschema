#include <sourcemeta/core/markdown.h>

#include <cmark-gfm-core-extensions.h> // cmark_gfm_core_extensions_ensure_registered
#include <cmark-gfm-extension_api.h> // cmark_find_syntax_extension, cmark_parser_attach_syntax_extension, cmark_parser_get_syntax_extensions
#include <cmark-gfm.h> // cmark_parser_new, cmark_parser_feed, cmark_parser_finish, cmark_parser_free, cmark_render_html, cmark_node_free

#include <array>   // std::array
#include <cstdlib> // std::free
#include <mutex>   // std::mutex, std::scoped_lock
#include <string>  // std::string

namespace sourcemeta::core {

auto markdown_to_html(const std::string_view input, const bool safe)
    -> std::string {
  [[maybe_unused]] static const bool cmark_initialized{
      (cmark_gfm_core_extensions_ensure_registered(), true)};

  // cmark-gfm toggles process-global special-character tables when syntax
  // extensions are attached and detached, so parser construction through
  // teardown cannot run concurrently
  static std::mutex cmark_mutex;
  const std::scoped_lock lock{cmark_mutex};

  static constexpr auto base_options{
      CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_FOOTNOTES |
      CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE | CMARK_OPT_GITHUB_PRE_LANG};
  // This cmark-gfm suppresses raw HTML and unsafe links by default, so raw
  // output requires explicitly opting out through CMARK_OPT_UNSAFE
  const int options{safe ? base_options : (base_options | CMARK_OPT_UNSAFE)};

  auto *parser{cmark_parser_new(options)};

  static constexpr std::array<const char *, 5> extension_names{
      {"table", "autolink", "strikethrough", "tagfilter", "tasklist"}};
  for (const auto *name : extension_names) {
    auto *extension{cmark_find_syntax_extension(name)};
    if (extension != nullptr) {
      cmark_parser_attach_syntax_extension(parser, extension);
    }
  }

  cmark_parser_feed(parser, input.data(), input.size());
  auto *document{cmark_parser_finish(parser)};
  auto *result{cmark_render_html(document, options,
                                 cmark_parser_get_syntax_extensions(parser))};

  std::string output{result};
  std::free(result);
  cmark_node_free(document);
  cmark_parser_free(parser);
  return output;
}

} // namespace sourcemeta::core
