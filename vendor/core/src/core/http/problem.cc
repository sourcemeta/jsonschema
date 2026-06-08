#include <sourcemeta/core/http_problem.h>

#include <cassert> // assert
#include <cstdint> // std::int64_t

namespace {

const auto HTTP_HASH_TYPE{sourcemeta::core::JSON::Object::hash("type")};
const auto HTTP_HASH_TITLE{sourcemeta::core::JSON::Object::hash("title")};
const auto HTTP_HASH_STATUS{sourcemeta::core::JSON::Object::hash("status")};
const auto HTTP_HASH_DETAIL{sourcemeta::core::JSON::Object::hash("detail")};
const auto HTTP_HASH_INSTANCE{sourcemeta::core::JSON::Object::hash("instance")};

} // namespace

namespace sourcemeta::core {

auto http_make_problem_details(const HTTPProblemDetails &problem)
    -> sourcemeta::core::JSON {
  assert(problem.status.code >= 100 && problem.status.code <= 599);
  assert(!problem.status.phrase.empty());
  assert(problem.type.find_first_of("\r\n") == JSON::StringView::npos);
  assert(problem.title.find_first_of("\r\n") == JSON::StringView::npos);
  assert(problem.instance.find_first_of("\r\n") == JSON::StringView::npos);

  auto object{sourcemeta::core::JSON::make_object()};
  object.assign_assume_new("type",
                           sourcemeta::core::JSON{problem.type.empty()
                                                      ? "about:blank"
                                                      : problem.type},
                           HTTP_HASH_TYPE);
  object.assign_assume_new("title",
                           sourcemeta::core::JSON{problem.title.empty()
                                                      ? problem.status.phrase
                                                      : problem.title},
                           HTTP_HASH_TITLE);
  object.assign_assume_new(
      "status",
      sourcemeta::core::JSON{static_cast<std::int64_t>(problem.status.code)},
      HTTP_HASH_STATUS);
  if (!problem.detail.empty()) {
    object.assign_assume_new("detail", sourcemeta::core::JSON{problem.detail},
                             HTTP_HASH_DETAIL);
  }
  if (!problem.instance.empty()) {
    object.assign_assume_new("instance",
                             sourcemeta::core::JSON{problem.instance},
                             HTTP_HASH_INSTANCE);
  }
  return object;
}

} // namespace sourcemeta::core
