#include <sourcemeta/core/build_adapter_filesystem.h>

#include <sourcemeta/core/io.h>

#include <cassert> // assert
#include <fstream> // std::ofstream

namespace sourcemeta::core {

BuildAdapterFilesystem::BuildAdapterFilesystem(std::string dependency_extension)
    : extension{std::move(dependency_extension)} {
  assert(!this->extension.empty());
  assert(this->extension.starts_with("."));
}

auto BuildAdapterFilesystem::dependencies_path(const node_type &path) const
    -> node_type {
  assert(path.is_absolute());
  return path.string() + this->extension;
}

auto BuildAdapterFilesystem::read_dependencies(const node_type &path) const
    -> std::optional<BuildDependencies<node_type>> {
  assert(path.is_absolute());
  const auto deps_path{this->dependencies_path(path)};
  if (std::filesystem::exists(deps_path)) {
    auto stream{sourcemeta::core::read_file(deps_path)};
    assert(stream.is_open());

    BuildDependencies<node_type> deps;
    std::string line;
    while (std::getline(stream, line)) {
      if (!line.empty()) {
        deps.emplace_back(line);
      }
    }

    if (deps.empty()) {
      return std::nullopt;
    } else {
      return deps;
    }
  } else {
    return std::nullopt;
  }
}

auto BuildAdapterFilesystem::write_dependencies(
    const node_type &path, const BuildDependencies<node_type> &dependencies)
    -> void {
  assert(path.is_absolute());
  assert(std::filesystem::exists(path));
  // Try to make sure as much as we can that any write operation made to disk
  flush(path);
  this->refresh(path);
  const auto deps_path{this->dependencies_path(path)};
  std::filesystem::create_directories(deps_path.parent_path());
  std::ofstream deps_stream{deps_path};
  assert(!deps_stream.fail());
  for (const auto &node : dependencies) {
    deps_stream << node.string() << "\n";
  }

  deps_stream.flush();
  deps_stream.close();
  flush(deps_path);
}

auto BuildAdapterFilesystem::refresh(const node_type &path) -> void {
  // We prefer our own computed marks so that we don't have to rely
  // too much on file-system specific non-sense
  const auto value{std::filesystem::file_time_type::clock::now()};
  // Because builds are typically ran in parallel
  std::lock_guard<std::mutex> lock{this->mutex};
  this->marks.insert_or_assign(path, value);
}

auto BuildAdapterFilesystem::mark(const node_type &path) const
    -> std::optional<mark_type> {
  assert(path.is_absolute());
  if (std::filesystem::exists(path)) {
    const auto match{this->marks.find(path)};
    if (match == this->marks.cend()) {
      // Keep in mind that depending on the OS, filesystem, and even standard
      // library implementation, this value might not be very reliable. In fact,
      // in many cases it can be outdated. Therefore, we never cache this value
      return std::filesystem::last_write_time(path);
    } else {
      return match->second;
    }
  } else {
    return std::nullopt;
  }
}

auto BuildAdapterFilesystem::is_newer_than(const mark_type left,
                                           const mark_type right) const
    -> bool {
  return left > right;
}

} // namespace sourcemeta::core
