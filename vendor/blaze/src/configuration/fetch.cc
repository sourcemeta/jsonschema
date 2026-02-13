#include <sourcemeta/blaze/configuration.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <cassert> // assert
#include <cstdint> // std::uint8_t
#include <sstream> // std::ostringstream
#include <utility> // std::move
#include <vector>  // std::vector

namespace {

auto compute_sha256(const std::string &content)
    -> sourcemeta::core::JSON::String {
  std::ostringstream result;
  sourcemeta::core::sha256(content, result);
  return result.str();
}

auto emit_event(
    const sourcemeta::blaze::Configuration::FetchEvent::Callback &callback,
    sourcemeta::blaze::Configuration::FetchEvent::Type type,
    const std::string &event_uri, const std::filesystem::path &event_path,
    std::size_t index, std::size_t total, std::string details = {},
    const std::exception_ptr &exception = nullptr,
    bool emit_error_if_aborted = false) -> bool {
  const auto result{callback({.type = type,
                              .uri = event_uri,
                              .path = event_path,
                              .index = index,
                              .total = total,
                              .details = std::move(details),
                              .exception = exception})};
  if (!result && emit_error_if_aborted) {
    emit_event(
        callback, sourcemeta::blaze::Configuration::FetchEvent::Type::Error,
        event_uri, event_path, index, total, "Operation aborted by callback");
  }

  return result;
}

enum class FetchResult : std::uint8_t { Success, Error, Aborted };

auto verify_written_schema(
    const std::string &dependency_uri,
    const std::filesystem::path &dependency_path,
    const sourcemeta::blaze::Configuration::ReadCallback &reader,
    const sourcemeta::blaze::Configuration::FetchEvent::Callback &callback,
    std::size_t index, std::size_t total,
    sourcemeta::core::JSON::String &out_hash) -> FetchResult {
  using FetchEvent = sourcemeta::blaze::Configuration::FetchEvent;

  if (!emit_event(callback, FetchEvent::Type::VerifyStart, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  std::string written_content;
  try {
    written_content = reader(dependency_path);
  } catch (...) {
    emit_event(callback, FetchEvent::Type::Error, dependency_uri,
               dependency_path, index, total, "Failed to verify written schema",
               std::current_exception());
    return FetchResult::Error;
  }

  out_hash = compute_sha256(written_content);

  if (!emit_event(callback, FetchEvent::Type::VerifyEnd, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  return FetchResult::Success;
}

auto fetch_and_write(
    const std::string &dependency_uri,
    const std::filesystem::path &dependency_path,
    const sourcemeta::blaze::Configuration::FetchCallback &fetcher,
    const sourcemeta::core::SchemaResolver &resolver,
    const sourcemeta::blaze::Configuration::WriteCallback &writer,
    const sourcemeta::blaze::Configuration::FetchEvent::Callback &callback,
    const std::optional<sourcemeta::core::JSON::String> &default_dialect,
    std::size_t index, std::size_t total, sourcemeta::core::JSON &out_schema)
    -> FetchResult {
  using FetchEvent = sourcemeta::blaze::Configuration::FetchEvent;

  if (!emit_event(callback, FetchEvent::Type::FetchStart, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  try {
    out_schema = fetcher(dependency_uri);
  } catch (...) {
    emit_event(callback, FetchEvent::Type::Error, dependency_uri,
               dependency_path, index, total, "Failed to fetch schema",
               std::current_exception());
    return FetchResult::Error;
  }

  if (!emit_event(callback, FetchEvent::Type::FetchEnd, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  if (!emit_event(callback, FetchEvent::Type::BundleStart, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  try {
    const std::string default_dialect_value{default_dialect.value_or("")};
    sourcemeta::core::bundle(out_schema, sourcemeta::core::schema_walker,
                             resolver, default_dialect_value, dependency_uri);
  } catch (...) {
    emit_event(callback, FetchEvent::Type::Error, dependency_uri,
               dependency_path, index, total, "Failed to bundle schema",
               std::current_exception());
    return FetchResult::Error;
  }

  if (!emit_event(callback, FetchEvent::Type::BundleEnd, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  if (!emit_event(callback, FetchEvent::Type::WriteStart, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  try {
    writer(dependency_path, out_schema);
  } catch (...) {
    emit_event(callback, FetchEvent::Type::Error, dependency_uri,
               dependency_path, index, total, "Failed to write schema",
               std::current_exception());
    return FetchResult::Error;
  }

  if (!emit_event(callback, FetchEvent::Type::WriteEnd, dependency_uri,
                  dependency_path, index, total, {}, nullptr, true)) {
    return FetchResult::Aborted;
  }

  return FetchResult::Success;
}

} // namespace

namespace sourcemeta::blaze {

auto Configuration::fetch(Lock &lock, const FetchCallback &fetcher,
                          const sourcemeta::core::SchemaResolver &resolver,
                          const ReadCallback &reader,
                          const WriteCallback &writer,
                          const FetchEvent::Callback &callback,
                          const FetchMode mode,
                          [[maybe_unused]] std::size_t concurrency) const
    -> void {
  const auto dependency_count{this->dependencies.size()};
  std::size_t current_index{0};

  for (const auto &[dependency_uri, dependency_path] : this->dependencies) {
    assert(dependency_path.is_absolute());
    const auto entry_status{
        lock.check(dependency_uri, dependency_path, reader)};

    bool should_fetch{false};
    switch (entry_status) {
      case Lock::Entry::Status::Untracked:
      case Lock::Entry::Status::FileMissing:
      case Lock::Entry::Status::Mismatched:
      case Lock::Entry::Status::PathMismatch:
        should_fetch = true;
        break;
      case Lock::Entry::Status::UpToDate:
        should_fetch = (mode == FetchMode::All);
        break;
    }

    if (should_fetch) {
      sourcemeta::core::JSON schema{sourcemeta::core::JSON::make_object()};
      const auto result{fetch_and_write(
          dependency_uri, dependency_path, fetcher, resolver, writer, callback,
          this->default_dialect, current_index, dependency_count, schema)};

      switch (result) {
        case FetchResult::Aborted:
        case FetchResult::Error:
          return;
        case FetchResult::Success:
          break;
      }

      sourcemeta::core::JSON::String written_hash;
      const auto verify_result{verify_written_schema(
          dependency_uri, dependency_path, reader, callback, current_index,
          dependency_count, written_hash)};

      switch (verify_result) {
        case FetchResult::Aborted:
        case FetchResult::Error:
          return;
        case FetchResult::Success:
          break;
      }

      lock.emplace(dependency_uri, dependency_path, written_hash);
    } else {
      if (!emit_event(callback, FetchEvent::Type::UpToDate, dependency_uri,
                      dependency_path, current_index, dependency_count, {},
                      nullptr, true)) {
        return;
      }
    }

    current_index += 1;
  }

  std::vector<sourcemeta::core::JSON::String> orphaned_uris;
  for (const auto &[lock_uri, lock_entry] : lock) {
    if (!this->dependencies.contains(lock_uri)) {
      orphaned_uris.push_back(lock_uri);
      if (!emit_event(callback, FetchEvent::Type::Orphaned, lock_uri,
                      lock_entry.path, 0, 0, {}, nullptr, true)) {
        return;
      }
    }
  }

  for (const auto &uri : orphaned_uris) {
    lock.erase(uri);
  }
}

auto Configuration::fetch(const Lock &lock, const FetchCallback &fetcher,
                          const sourcemeta::core::SchemaResolver &resolver,
                          const ReadCallback &reader,
                          const WriteCallback &writer,
                          const FetchEvent::Callback &callback,
                          const bool dry_run,
                          [[maybe_unused]] std::size_t concurrency) const
    -> void {
  const auto dependency_count{this->dependencies.size()};
  std::size_t current_index{0};

  for (const auto &[dependency_uri, dependency_path] : this->dependencies) {
    assert(dependency_path.is_absolute());
    const auto entry_status{
        lock.check(dependency_uri, dependency_path, reader)};
    switch (entry_status) {
      case Lock::Entry::Status::Untracked:
        if (!emit_event(callback, FetchEvent::Type::Untracked, dependency_uri,
                        dependency_path, current_index, dependency_count, {},
                        nullptr, true)) {
          return;
        }
        break;

      case Lock::Entry::Status::FileMissing: {
        if (dry_run) {
          if (!emit_event(callback, FetchEvent::Type::FileMissing,
                          dependency_uri, dependency_path, current_index,
                          dependency_count, {}, nullptr, true)) {
            return;
          }
        } else {
          sourcemeta::core::JSON schema{sourcemeta::core::JSON::make_object()};
          const auto result{
              fetch_and_write(dependency_uri, dependency_path, fetcher,
                              resolver, writer, callback, this->default_dialect,
                              current_index, dependency_count, schema)};

          switch (result) {
            case FetchResult::Aborted:
            case FetchResult::Error:
              return;
            case FetchResult::Success:
              break;
          }

          sourcemeta::core::JSON::String written_hash;
          const auto verify_result{verify_written_schema(
              dependency_uri, dependency_path, reader, callback, current_index,
              dependency_count, written_hash)};

          switch (verify_result) {
            case FetchResult::Aborted:
            case FetchResult::Error:
              return;
            case FetchResult::Success:
              break;
          }

          assert(lock.at(dependency_uri).has_value());
          const auto &lock_entry{lock.at(dependency_uri)->get()};
          if (written_hash != lock_entry.hash) {
            emit_event(callback, FetchEvent::Type::Error, dependency_uri,
                       dependency_path, current_index, dependency_count,
                       "Written file hash does not match lock file");
            return;
          }
        }
        break;
      }

      case Lock::Entry::Status::Mismatched:
        if (!emit_event(callback, FetchEvent::Type::Mismatched, dependency_uri,
                        dependency_path, current_index, dependency_count, {},
                        nullptr, true)) {
          return;
        }
        if (!dry_run) {
          emit_event(callback, FetchEvent::Type::Error, dependency_uri,
                     dependency_path, current_index, dependency_count,
                     "File hash does not match lock file in frozen mode");
          return;
        }
        break;

      case Lock::Entry::Status::PathMismatch:
        if (!emit_event(callback, FetchEvent::Type::PathMismatch,
                        dependency_uri, dependency_path, current_index,
                        dependency_count, {}, nullptr, true)) {
          return;
        }
        if (!dry_run) {
          emit_event(callback, FetchEvent::Type::Error, dependency_uri,
                     dependency_path, current_index, dependency_count,
                     "Configured path does not match lock file in frozen mode");
          return;
        }
        break;

      case Lock::Entry::Status::UpToDate:
        if (!emit_event(callback, FetchEvent::Type::UpToDate, dependency_uri,
                        dependency_path, current_index, dependency_count, {},
                        nullptr, true)) {
          return;
        }
        break;
    }

    current_index += 1;
  }

  for (const auto &[lock_uri, lock_entry] : lock) {
    if (!this->dependencies.contains(lock_uri)) {
      if (!emit_event(callback, FetchEvent::Type::Orphaned, lock_uri,
                      lock_entry.path, 0, 0, {}, nullptr, true)) {
        return;
      }
    }
  }
}

} // namespace sourcemeta::blaze
