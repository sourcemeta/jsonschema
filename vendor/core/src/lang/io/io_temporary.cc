#include <sourcemeta/core/io_temporary.h>

#include <cassert>    // assert
#include <filesystem> // std::filesystem
#include <string>     // std::string
#include <system_error> // std::error_code, std::generic_category, std::make_error_code

#if defined(_WIN32)
#include <io.h> // _mktemp_s
#else
#include <cerrno>   // errno
#include <unistd.h> // mkdtemp
#endif

namespace sourcemeta::core {

TemporaryDirectory::TemporaryDirectory(const std::filesystem::path &parent,
                                       const std::string_view prefix) {
  assert(!prefix.empty());
  assert(prefix.find('/') == std::string_view::npos);
  assert(prefix.find('\\') == std::string_view::npos);
  if (std::filesystem::exists(parent) &&
      !std::filesystem::is_directory(parent)) {
    throw std::filesystem::filesystem_error{
        "parent path exists but is not a directory", parent,
        std::make_error_code(std::errc::not_a_directory)};
  }

  std::filesystem::create_directories(parent);
  auto name{(parent / std::string{prefix}).string() + "XXXXXX"};

#if defined(_WIN32)
  const auto error{_mktemp_s(name.data(), name.size() + 1)};
  if (error != 0) {
    throw std::filesystem::filesystem_error{
        "failed to create temporary directory", parent,
        std::error_code{error, std::generic_category()}};
  }

  if (!std::filesystem::create_directory(name)) {
    throw std::filesystem::filesystem_error{
        "failed to create temporary directory", parent,
        std::make_error_code(std::errc::file_exists)};
  }
#else
  if (mkdtemp(name.data()) == nullptr) {
    throw std::filesystem::filesystem_error{
        "failed to create temporary directory", parent,
        std::error_code{errno, std::generic_category()}};
  }
#endif

  this->path_ = std::filesystem::path{name};
}

TemporaryDirectory::~TemporaryDirectory() {
  std::error_code error;
  std::filesystem::remove_all(this->path_, error);
}

auto TemporaryDirectory::path() const noexcept
    -> const std::filesystem::path & {
  return this->path_;
}

} // namespace sourcemeta::core
