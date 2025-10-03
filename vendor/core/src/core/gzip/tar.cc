#include <sourcemeta/core/gzip_tar.h>

#include <sourcemeta/core/gzip_error.h> // GZIPArchiveError

#include <archive.h>       // archive_write_new, archive_write_close, etc.
#include <archive_entry.h> // archive_entry_new, archive_entry_set_*, etc.

#include <cstdlib>     // std::malloc, std::free
#include <cstring>     // std::cmp_not_equal
#include <ctime>       // std::time
#include <filesystem>  // std::filesystem::path
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::cmp_not_equal

namespace {

// Anonymous namespace function to finalize the archive
auto finalize_archive(struct archive *archive, bool &finalized) -> void {
  if (finalized) {
    return;
  }

  if (archive_write_close(archive) != ARCHIVE_OK) {
    throw sourcemeta::core::GZIPArchiveError{"Failed to close archive"};
  }

  finalized = true;
}

} // anonymous namespace

namespace sourcemeta::core {

// PIMPL implementation
struct GZIPTar::Implementation {
  struct archive *archive_{nullptr};
  char *buffer_{nullptr};
  std::size_t buffer_size_{0};
  std::size_t used_size_{0};
  bool finalized_{false};
};

GZIPTar::GZIPTar() : internal_{std::make_unique<Implementation>()} {
  // Create a new archive
  this->internal_->archive_ = archive_write_new();
  if (!this->internal_->archive_) {
    throw GZIPArchiveError{"Failed to create archive"};
  }

  // Set the format to TAR and add gzip compression
  if (archive_write_add_filter_gzip(this->internal_->archive_) != ARCHIVE_OK) {
    archive_write_free(this->internal_->archive_);
    throw GZIPArchiveError{"Failed to add gzip filter"};
  }

  if (archive_write_set_format_pax_restricted(this->internal_->archive_) !=
      ARCHIVE_OK) {
    archive_write_free(this->internal_->archive_);
    throw GZIPArchiveError{"Failed to set TAR format"};
  }

  // 64KB initial buffer
  this->internal_->buffer_size_ = 65536;
  this->internal_->buffer_ =
      static_cast<char *>(std::malloc(this->internal_->buffer_size_));
  if (!this->internal_->buffer_) {
    archive_write_free(this->internal_->archive_);
    throw GZIPArchiveError{"Failed to allocate memory buffer"};
  }

  if (archive_write_open_memory(this->internal_->archive_,
                                this->internal_->buffer_,
                                this->internal_->buffer_size_,
                                &this->internal_->used_size_) != ARCHIVE_OK) {
    std::free(this->internal_->buffer_);
    archive_write_free(this->internal_->archive_);
    throw GZIPArchiveError{"Failed to open archive"};
  }
}

GZIPTar::~GZIPTar() {
  if (this->internal_->archive_) {
    if (!this->internal_->finalized_) {
      try {
        finalize_archive(this->internal_->archive_,
                         this->internal_->finalized_);
        // NOLINTNEXTLINE(bugprone-empty-catch)
      } catch (...) {
        // Ignore exceptions in destructor to prevent termination
        // as throwing from a destructor can lead to undefined behavior
      }
    }

    archive_write_free(this->internal_->archive_);
  }

  if (this->internal_->buffer_) {
    std::free(this->internal_->buffer_);
  }
}

auto GZIPTar::push(const std::filesystem::path &path,
                   const std::string_view content) -> void {
  this->push(path, content.data(), content.size());
}

auto GZIPTar::push(const std::filesystem::path &path, const void *data,
                   const std::size_t size) -> void {
  if (this->internal_->finalized_) {
    throw GZIPArchiveError{"Cannot add files to finalized archive"};
  }

  struct archive_entry *entry = archive_entry_new();
  if (!entry) {
    throw GZIPArchiveError{"Failed to create archive entry"};
  }

  // Set file properties
  archive_entry_set_pathname(entry, path.string().c_str());
  archive_entry_set_size(entry, static_cast<la_int64_t>(size));
  archive_entry_set_filetype(entry, AE_IFREG);
  archive_entry_set_perm(entry, 0644);

  // Set timestamps to current time
  const std::time_t now{std::time(nullptr)};
  archive_entry_set_mtime(entry, now, 0);
  archive_entry_set_ctime(entry, now, 0);
  archive_entry_set_atime(entry, now, 0);

  // Write the header
  if (archive_write_header(this->internal_->archive_, entry) != ARCHIVE_OK) {
    archive_entry_free(entry);
    throw GZIPArchiveError{"Failed to write header"};
  }

  // Write the file content
  if (size > 0 && data != nullptr) {
    const la_ssize_t written =
        archive_write_data(this->internal_->archive_, data, size);
    if (written < 0) {
      archive_entry_free(entry);
      throw GZIPArchiveError{"Failed to write data"};
    }

    if (std::cmp_not_equal(written, size)) {
      archive_entry_free(entry);
      throw GZIPArchiveError{"Incomplete write"};
    }
  }

  archive_entry_free(entry);
}

auto GZIPTar::data() -> const char * {
  if (!this->internal_->finalized_) {
    finalize_archive(this->internal_->archive_, this->internal_->finalized_);
  }

  return this->internal_->buffer_;
}

auto GZIPTar::size() -> std::size_t {
  if (!this->internal_->finalized_) {
    finalize_archive(this->internal_->archive_, this->internal_->finalized_);
  }

  return this->internal_->used_size_;
}

} // namespace sourcemeta::core
