#ifndef SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_
#define SOURCEMETA_JSONSCHEMA_CLI_UTILS_H_

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonpointer.h>
#include <sourcemeta/core/options.h>
#include <sourcemeta/core/schemaconfig.h>

#include <sourcemeta/blaze/output.h>

#include <cassert>  // assert
#include <iterator> // std::next
#include <optional> // std::optional
#include <ostream>  // std::ostream
#include <string>   // std::string, std::stoull
#include <tuple>    // std::get

namespace sourcemeta::jsonschema {

inline auto default_dialect(
    const sourcemeta::core::Options &options,
    const std::optional<sourcemeta::core::SchemaConfig> &configuration)
    -> std::optional<std::string> {
  if (options.contains("default-dialect")) {
    return std::string{options.at("default-dialect").front()};
  } else if (configuration.has_value()) {
    return configuration.value().default_dialect;
  }

  return std::nullopt;
}

inline auto parse_indentation(const sourcemeta::core::Options &options)
    -> std::size_t {
  if (options.contains("indentation")) {
    return std::stoull(std::string{options.at("indentation").front()});
  }

  return 2;
}

inline auto print(const sourcemeta::blaze::SimpleOutput &output,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream) -> void {
  stream << "error: Schema validation failure\n";
  for (const auto &entry : output) {
    stream << "  " << entry.message << "\n";
    stream << "    at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"";

    const auto position{
        tracker.get(sourcemeta::core::to_pointer(entry.instance_location))};
    if (position.has_value()) {
      stream << " (line " << std::get<0>(position.value()) << ", column "
             << std::get<1>(position.value()) << ")";
    }

    stream << "\n";
    stream << "    at evaluate path \"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\"\n";
  }
}

inline auto
print_annotations(const sourcemeta::blaze::SimpleOutput &output,
                  const sourcemeta::core::Options &options,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream) -> void {
  if (options.contains("verbose")) {
    for (const auto &annotation : output.annotations()) {
      for (const auto &value : annotation.second) {
        stream << "annotation: ";
        sourcemeta::core::stringify(value, stream);
        stream << "\n  at instance location \"";
        sourcemeta::core::stringify(annotation.first.instance_location, stream);
        stream << "\"";

        const auto position{tracker.get(
            sourcemeta::core::to_pointer(annotation.first.instance_location))};
        if (position.has_value()) {
          stream << " (line " << std::get<0>(position.value()) << ", column "
                 << std::get<1>(position.value()) << ")";
        }

        stream << "\n  at evaluate path \"";
        sourcemeta::core::stringify(annotation.first.evaluate_path, stream);
        stream << "\"\n";
      }
    }
  }
}

inline auto print(const sourcemeta::blaze::TraceOutput &output,
                  const sourcemeta::core::PointerPositionTracker &tracker,
                  std::ostream &stream) -> void {
  for (auto iterator = output.cbegin(); iterator != output.cend(); iterator++) {
    const auto &entry{*iterator};

    if (entry.evaluate_path.empty()) {
      continue;
    }

    switch (entry.type) {
      case sourcemeta::blaze::TraceOutput::EntryType::Push:
        stream << "-> (push) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Pass:
        stream << "<- (pass) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Fail:
        stream << "<- (fail) ";
        break;
      case sourcemeta::blaze::TraceOutput::EntryType::Annotation:
        stream << "@- (annotation) ";
        break;
      default:
        assert(false);
        break;
    }

    stream << "\"";
    sourcemeta::core::stringify(entry.evaluate_path, stream);
    stream << "\"";
    stream << " (" << entry.name << ")\n";

    if (entry.annotation.has_value()) {
      stream << "   value ";

      if (entry.annotation.value().is_object()) {
        sourcemeta::core::stringify(entry.annotation.value(), stream);
      } else {
        sourcemeta::core::prettify(entry.annotation.value(), stream);
      }

      stream << "\n";
    }

    stream << "   at instance location \"";
    sourcemeta::core::stringify(entry.instance_location, stream);
    stream << "\"";

    const auto position{
        tracker.get(sourcemeta::core::to_pointer(entry.instance_location))};
    if (position.has_value()) {
      stream << " (line " << std::get<0>(position.value()) << ", column "
             << std::get<1>(position.value()) << ")";
    }

    stream << "\n";
    stream << "   at keyword location \"" << entry.keyword_location << "\"\n";

    if (entry.vocabulary.first) {
      stream << "   at vocabulary \""
             << entry.vocabulary.second.value_or("<unknown>") << "\"\n";
    }

    // To make it easier to read
    if (std::next(iterator) != output.cend()) {
      stream << "\n";
    }
  }
}

} // namespace sourcemeta::jsonschema

#endif
