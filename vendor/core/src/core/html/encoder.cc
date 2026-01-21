#include <sourcemeta/core/html_encoder.h>

#include <iostream> // std::ostream
#include <sstream>  // std::ostringstream
#include <string>   // std::string

namespace sourcemeta::core {

auto HTML::render() const -> std::string {
  std::ostringstream output_stream;
  output_stream << "<" << this->tag_name;

  // Render attributes
  for (const auto &[attribute_name, attribute_value] : this->attributes) {
    std::string escaped_value{attribute_value};
    html_escape(escaped_value);
    output_stream << " " << attribute_name << "=\"" << escaped_value << "\"";
  }

  if (this->self_closing) {
    output_stream << " />";
    return output_stream.str();
  }

  output_stream << ">";

  // Render children
  if (this->child_elements.empty()) {
    output_stream << "</" << this->tag_name << ">";
  } else if (this->child_elements.size() == 1 &&
             std::get_if<std::string>(&this->child_elements[0])) {
    // Inline single text node
    output_stream << this->render(this->child_elements[0]);
    output_stream << "</" << this->tag_name << ">";
  } else {
    // Block level children
    for (const auto &child_element : this->child_elements) {
      output_stream << this->render(child_element);
    }
    output_stream << "</" << this->tag_name << ">";
  }

  return output_stream.str();
}

auto HTML::render(const HTMLNode &child_element) const -> std::string {
  if (const auto *text = std::get_if<std::string>(&child_element)) {
    std::string escaped_text{*text};
    html_escape(escaped_text);
    return escaped_text;
  } else if (const auto *raw_html = std::get_if<HTMLRaw>(&child_element)) {
    return raw_html->content;
  } else if (const auto *html_element = std::get_if<HTML>(&child_element)) {
    return html_element->render();
  }
  return "";
}

auto HTML::push_back(const HTMLNode &child) -> HTML & {
  this->child_elements.push_back(child);
  return *this;
}

auto HTML::push_back(HTMLNode &&child) -> HTML & {
  this->child_elements.push_back(std::move(child));
  return *this;
}

auto operator<<(std::ostream &output_stream, const HTML &html_element)
    -> std::ostream & {
  return output_stream << html_element.render();
}

} // namespace sourcemeta::core
