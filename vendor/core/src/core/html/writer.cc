#include <sourcemeta/core/html_writer.h>

#include <iostream> // std::ostream

namespace sourcemeta::core {

auto HTMLBuffer::grow(const std::size_t needed) -> void {
  const auto current_size{
      this->cursor_
          ? static_cast<std::size_t>(this->cursor_ - this->buffer_.data())
          : 0};
  auto new_capacity{this->buffer_.empty() ? std::size_t{1024}
                                          : this->buffer_.size() * 2};
  while (new_capacity < current_size + needed) {
    new_capacity *= 2;
  }

  this->buffer_.resize(new_capacity);
  this->cursor_ = this->buffer_.data() + current_size;
  this->end_ = this->buffer_.data() + new_capacity;
}

auto HTMLBuffer::write(std::ostream &stream) -> void {
  if (this->cursor_) {
    const auto size{
        static_cast<std::size_t>(this->cursor_ - this->buffer_.data())};
    stream.write(this->buffer_.data(), static_cast<std::streamsize>(size));
  }
}

auto HTMLWriter::flush_open_tag() -> void {
  if (this->tag_open_) {
    if (this->tag_open_is_void_) {
      this->buffer_.append(" />");
    } else {
      this->buffer_.append(">");
    }

    this->tag_open_ = false;
    this->tag_open_is_void_ = false;
  }
}

auto HTMLWriter::write(std::ostream &stream) -> void {
  this->flush_open_tag();
  this->buffer_.write(stream);
}

} // namespace sourcemeta::core
