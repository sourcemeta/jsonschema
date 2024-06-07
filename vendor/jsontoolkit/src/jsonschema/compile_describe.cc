#include <sourcemeta/jsontoolkit/jsonschema_compile.h>

#include <variant> // std::visit

namespace {
using namespace sourcemeta::jsontoolkit;

struct DescribeVisitor {
  auto operator()(const SchemaCompilerLogicalOr &) const -> std::string {
    return "The target is expected to match at least one of the given "
           "assertions";
  }
  auto operator()(const SchemaCompilerLogicalAnd &) const -> std::string {
    return "The target is expected to match all of the given assertions";
  }
  auto operator()(const SchemaCompilerLogicalXor &) const -> std::string {
    return "The target is expected to match one and only one of the given "
           "assertions";
  }
  auto operator()(const SchemaCompilerLogicalNot &) const -> std::string {
    return "The given schema is expected to not validate successfully";
  }
  auto
  operator()(const SchemaCompilerInternalContainer &) const -> std::string {
    return "Internal";
  }
  auto
  operator()(const SchemaCompilerInternalNoAnnotation &) const -> std::string {
    return "The target was not annotated with the given value";
  }
  auto
  operator()(const SchemaCompilerInternalDefinesAll &) const -> std::string {
    return "The target object is expected to define all of the given "
           "properties";
  }
  auto operator()(const SchemaCompilerControlLabel &) const -> std::string {
    return "Mark the current position of the evaluation process for future "
           "jumps";
  }
  auto operator()(const SchemaCompilerControlJump &) const -> std::string {
    return "Jump to another point of the evaluation process";
  }
  auto operator()(const SchemaCompilerAnnotationPublic &) const -> std::string {
    return "Emit an annotation";
  }
  auto
  operator()(const SchemaCompilerAnnotationPrivate &) const -> std::string {
    return "Emit an internal annotation";
  }
  auto operator()(const SchemaCompilerLoopProperties &) const -> std::string {
    return "Loop over the properties of the target object";
  }
  auto operator()(const SchemaCompilerLoopKeys &) const -> std::string {
    return "Loop over the property keys of the target object";
  }
  auto operator()(const SchemaCompilerLoopItems &) const -> std::string {
    return "Loop over the items of the target array";
  }
  auto operator()(const SchemaCompilerLoopContains &) const -> std::string {
    return "A certain number of array items must satisfy the given constraints";
  }
  auto operator()(const SchemaCompilerAssertionFail &) const -> std::string {
    return "Abort evaluation on failure";
  }
  auto operator()(const SchemaCompilerAssertionDefines &) const -> std::string {
    return "The target object is expected to define the given property";
  }
  auto
  operator()(const SchemaCompilerAssertionDefinesAll &) const -> std::string {
    return "The target object is expected to define all of the given "
           "properties";
  }
  auto operator()(const SchemaCompilerAssertionType &) const -> std::string {
    return "The target document is expected to be of the given type";
  }
  auto operator()(const SchemaCompilerAssertionTypeAny &) const -> std::string {
    return "The target document is expected to be of one of the given types";
  }
  auto
  operator()(const SchemaCompilerAssertionTypeStrict &) const -> std::string {
    return "The target document is expected to be of the given type";
  }
  auto operator()(const SchemaCompilerAssertionTypeStrictAny &) const
      -> std::string {
    return "The target document is expected to be of one of the given types";
  }
  auto operator()(const SchemaCompilerAssertionRegex &) const -> std::string {
    return "The target string is expected to match the given regular "
           "expression";
  }
  auto
  operator()(const SchemaCompilerAssertionSizeGreater &) const -> std::string {
    return "The target size is expected to be greater than the given number";
  }
  auto
  operator()(const SchemaCompilerAssertionSizeLess &) const -> std::string {
    return "The target size is expected to be less than the given number";
  }

  auto operator()(const SchemaCompilerAssertionEqual &) const -> std::string {
    return "The target size is expected to be equal to the given number";
  }
  auto operator()(const SchemaCompilerAssertionGreaterEqual &) const {
    return "The target number is expected to be greater than or equal to the "
           "given number";
  }
  auto
  operator()(const SchemaCompilerAssertionLessEqual &) const -> std::string {
    return "The target number is expected to be less than or equal to the "
           "given number";
  }
  auto operator()(const SchemaCompilerAssertionGreater &) const -> std::string {
    return "The target number is expected to be greater than the given number";
  }
  auto operator()(const SchemaCompilerAssertionLess &) const -> std::string {
    return "The target number is expected to be less than the given number";
  }
  auto operator()(const SchemaCompilerAssertionUnique &) const -> std::string {
    return "The target array is expected to not contain duplicates";
  }
  auto
  operator()(const SchemaCompilerAssertionDivisible &) const -> std::string {
    return "The target number is expected to be divisible by the given number";
  }
  auto
  operator()(const SchemaCompilerAssertionStringType &) const -> std::string {
    return "The target string is expected to match the given logical type";
  }
  auto
  operator()(const SchemaCompilerAssertionEqualsAny &) const -> std::string {
    return "The target document is expected to be one of the given values";
  }
};

} // namespace

namespace sourcemeta::jsontoolkit {

auto describe(const SchemaCompilerTemplate::value_type &step) -> std::string {
  return std::visit<std::string>(DescribeVisitor{}, step);
}

} // namespace sourcemeta::jsontoolkit
