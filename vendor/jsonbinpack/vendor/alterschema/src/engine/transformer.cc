#include <sourcemeta/alterschema/engine.h>

#include <utility> // std::move

sourcemeta::alterschema::Transformer::Transformer(
    sourcemeta::jsontoolkit::JSON &schema)
    : data{schema} {}

auto sourcemeta::alterschema::Transformer::schema() const
    -> const sourcemeta::jsontoolkit::JSON & {
  return this->data;
}

auto sourcemeta::alterschema::Transformer::replace(
    const sourcemeta::jsontoolkit::Pointer &path,
    const sourcemeta::jsontoolkit::JSON &value) -> void {
  // TODO: Check that the path exists with an assert
  sourcemeta::jsontoolkit::set(this->data, path, value);
  this->operations.push_back(OperationReplace{path});
}

auto sourcemeta::alterschema::Transformer::replace(
    const sourcemeta::jsontoolkit::Pointer &path,
    sourcemeta::jsontoolkit::JSON &&value) -> void {
  // TODO: Check that the path exists with an assert
  sourcemeta::jsontoolkit::set(this->data, path, std::move(value));
  this->operations.push_back(OperationReplace{path});
}

auto sourcemeta::alterschema::Transformer::replace(
    const sourcemeta::jsontoolkit::JSON &value) -> void {
  this->replace(sourcemeta::jsontoolkit::empty_pointer, value);
}

auto sourcemeta::alterschema::Transformer::replace(
    sourcemeta::jsontoolkit::JSON &&value) -> void {
  this->replace(sourcemeta::jsontoolkit::empty_pointer, std::move(value));
}

auto sourcemeta::alterschema::Transformer::erase(
    const sourcemeta::jsontoolkit::Pointer &path,
    const sourcemeta::jsontoolkit::JSON::String &key) -> void {
  // TODO: Check that the path exists with an assert
  sourcemeta::jsontoolkit::get(this->data, path).erase(key);
  this->operations.push_back(OperationErase{path.concat({key})});
}

auto sourcemeta::alterschema::Transformer::erase(
    const sourcemeta::jsontoolkit::JSON::String &key) -> void {
  this->erase(sourcemeta::jsontoolkit::empty_pointer, key);
}

auto sourcemeta::alterschema::Transformer::assign(
    const sourcemeta::jsontoolkit::Pointer &path,
    const sourcemeta::jsontoolkit::JSON::String &key,
    const sourcemeta::jsontoolkit::JSON &value) -> void {
  const auto destination{path.concat({key})};
  // TODO: Check that the path DOES NOT exist with an assert
  sourcemeta::jsontoolkit::get(this->data, path).assign(key, value);
  this->operations.push_back(OperationAssign{path.concat({key})});
}

auto sourcemeta::alterschema::Transformer::assign(
    const sourcemeta::jsontoolkit::Pointer &path,
    const sourcemeta::jsontoolkit::JSON::String &key,
    sourcemeta::jsontoolkit::JSON &&value) -> void {
  // TODO: Check that the path DOES NOT exist with an assert
  sourcemeta::jsontoolkit::get(this->data, path).assign(key, std::move(value));
  this->operations.push_back(OperationAssign{path.concat({key})});
}

auto sourcemeta::alterschema::Transformer::assign(
    const sourcemeta::jsontoolkit::JSON::String &key,
    const sourcemeta::jsontoolkit::JSON &value) -> void {
  this->assign(sourcemeta::jsontoolkit::empty_pointer, key, value);
}

auto sourcemeta::alterschema::Transformer::assign(
    const sourcemeta::jsontoolkit::JSON::String &key,
    sourcemeta::jsontoolkit::JSON &&value) -> void {
  this->assign(sourcemeta::jsontoolkit::empty_pointer, key, std::move(value));
}

auto sourcemeta::alterschema::Transformer::traces() const
    -> const std::vector<Operation> & {
  return this->operations;
}
