#include <sourcemeta/blaze/configuration.h>

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>

#include <cassert> // assert
#include <sstream> // std::ostringstream
#include <utility> // std::move

namespace {

auto compute_hash(
    const std::string &content,
    const sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm
        algorithm,
    const sourcemeta::core::Pointer &location)
    -> sourcemeta::core::JSON::String {
  using HashAlgorithm =
      sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm;
  switch (algorithm) {
    case HashAlgorithm::SHA256: {
      std::ostringstream result;
      sourcemeta::core::sha256(content, result);
      return result.str();
    }
    default:
      throw sourcemeta::blaze::ConfigurationParseError("Unknown hash algorithm",
                                                       location);
  }
}

auto hash_algorithm_to_string(
    const sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm
        algorithm,
    const sourcemeta::core::Pointer &location)
    -> sourcemeta::core::JSON::String {
  using HashAlgorithm =
      sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm;
  switch (algorithm) {
    case HashAlgorithm::SHA256:
      return "sha256";
    default:
      throw sourcemeta::blaze::ConfigurationParseError("Unknown hash algorithm",
                                                       location);
  }
}

auto string_to_hash_algorithm(const sourcemeta::core::JSON::String &value,
                              const sourcemeta::core::Pointer &location)
    -> sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm {
  using HashAlgorithm =
      sourcemeta::blaze::Configuration::Lock::Entry::HashAlgorithm;
  if (value == "sha256") {
    return HashAlgorithm::SHA256;
  }

  throw sourcemeta::blaze::ConfigurationParseError("Unknown hash algorithm",
                                                   location);
}

} // namespace

namespace sourcemeta::blaze {

auto Configuration::Lock::from_json(const sourcemeta::core::JSON &value)
    -> Lock {
  Lock result;

  if (!value.is_object()) {
    throw ConfigurationParseError("The lock file must be an object", {});
  }

  if (!value.defines("version")) {
    throw ConfigurationParseError("The lock file must have a version property",
                                  {});
  }

  if (!value.at("version").is_integer() ||
      value.at("version").to_integer() != 1) {
    throw ConfigurationParseError("Unsupported lock file version", {"version"});
  }

  if (value.defines("dependencies")) {
    if (!value.at("dependencies").is_object()) {
      throw ConfigurationParseError(
          "The lock file dependencies property must be an object",
          {"dependencies"});
    }

    for (const auto &pair : value.at("dependencies").as_object()) {
      if (!pair.second.is_object()) {
        throw ConfigurationParseError(
            "The lock file dependency entry must be an object",
            {"dependencies", pair.first});
      }

      if (!pair.second.defines("path") || !pair.second.at("path").is_string()) {
        throw ConfigurationParseError(
            "The lock file dependency entry must have a path",
            {"dependencies", pair.first, "path"});
      }

      if (!pair.second.defines("hash") || !pair.second.at("hash").is_string()) {
        throw ConfigurationParseError(
            "The lock file dependency entry must have a hash",
            {"dependencies", pair.first, "hash"});
      }

      if (!pair.second.defines("hashAlgorithm") ||
          !pair.second.at("hashAlgorithm").is_string()) {
        throw ConfigurationParseError(
            "The lock file dependency entry must have a hash algorithm",
            {"dependencies", pair.first, "hashAlgorithm"});
      }

      const std::filesystem::path entry_path{
          pair.second.at("path").to_string()};
      if (!entry_path.is_absolute()) {
        throw ConfigurationParseError(
            "The lock file dependency entry path must be absolute",
            {"dependencies", pair.first, "path"});
      }

      Entry entry;
      entry.path = entry_path;
      entry.hash = pair.second.at("hash").to_string();
      entry.hash_algorithm = string_to_hash_algorithm(
          pair.second.at("hashAlgorithm").to_string(),
          {"dependencies", pair.first, "hashAlgorithm"});

      result.entries_.emplace(pair.first, std::move(entry));
    }
  }

  return result;
}

auto Configuration::Lock::to_json() const -> sourcemeta::core::JSON {
  auto result{sourcemeta::core::JSON::make_object()};
  result.assign("version", sourcemeta::core::JSON{1});

  auto dependencies_json{sourcemeta::core::JSON::make_object()};
  for (const auto &pair : this->entries_) {
    assert(pair.second.path.is_absolute());
    auto entry_json{sourcemeta::core::JSON::make_object()};
    entry_json.assign(
        "path", sourcemeta::core::JSON{pair.second.path.generic_string()});
    entry_json.assign("hash", sourcemeta::core::JSON{pair.second.hash});
    entry_json.assign("hashAlgorithm",
                      sourcemeta::core::JSON{hash_algorithm_to_string(
                          pair.second.hash_algorithm,
                          {"dependencies", pair.first, "hashAlgorithm"})});
    dependencies_json.assign(pair.first, std::move(entry_json));
  }

  result.assign("dependencies", std::move(dependencies_json));
  return result;
}

auto Configuration::Lock::emplace(const sourcemeta::core::JSON::String &uri,
                                  const std::filesystem::path &path,
                                  const sourcemeta::core::JSON::String &hash,
                                  Entry::HashAlgorithm hash_algorithm) -> void {
  assert(path.is_absolute());
  Entry entry;
  entry.path = path;
  entry.hash = hash;
  entry.hash_algorithm = hash_algorithm;
  this->entries_.insert_or_assign(uri, std::move(entry));
}

auto Configuration::Lock::size() const noexcept -> std::size_t {
  return this->entries_.size();
}

auto Configuration::Lock::at(const sourcemeta::core::JSON::String &uri) const
    -> std::optional<std::reference_wrapper<const Entry>> {
  const auto iterator{this->entries_.find(uri)};
  if (iterator == this->entries_.cend()) {
    return std::nullopt;
  }

  return std::cref(iterator->second);
}

auto Configuration::Lock::begin() const noexcept -> const_iterator {
  return this->entries_.cbegin();
}

auto Configuration::Lock::end() const noexcept -> const_iterator {
  return this->entries_.cend();
}

auto Configuration::Lock::check(const sourcemeta::core::JSON::String &uri,
                                const std::filesystem::path &expected_path,
                                const Configuration::ReadCallback &reader) const
    -> Entry::Status {
  const auto iterator{this->entries_.find(uri)};
  if (iterator == this->entries_.cend()) {
    return Entry::Status::Untracked;
  }

  const auto &entry{iterator->second};

  if (entry.path != expected_path) {
    return Entry::Status::PathMismatch;
  }

  std::string content;
  try {
    content = reader(entry.path);
  } catch (...) {
    return Entry::Status::FileMissing;
  }

  const auto current_hash{
      compute_hash(content, entry.hash_algorithm, {"dependencies", uri})};
  if (current_hash != entry.hash) {
    return Entry::Status::Mismatched;
  }

  return Entry::Status::UpToDate;
}

} // namespace sourcemeta::blaze
