#include <sourcemeta/core/markdown.h>

#include <cmark-gfm-core-extensions.h> // cmark_gfm_core_extensions_ensure_registered
#include <cmark-gfm-extension_api.h> // cmark_find_syntax_extension, cmark_parser_attach_syntax_extension, cmark_parser_get_syntax_extensions
#include <cmark-gfm.h> // cmark_parser_new, cmark_parser_feed, cmark_parser_finish, cmark_parser_free, cmark_render_html, cmark_node_free

#include <array>   // std::array
#include <cstdlib> // std::free
#include <string>  // std::string

namespace sourcemeta::core {

auto markdown_to_html(const std::string_view input) -> std::string {
  static constexpr auto options{CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_FOOTNOTES |
                                CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE |
                                CMARK_OPT_GITHUB_PRE_LANG};
  cmark_gfm_core_extensions_ensure_registered();

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
