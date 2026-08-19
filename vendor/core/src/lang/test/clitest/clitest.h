#ifndef SOURCEMETA_CORE_TEST_CLITEST_H_
#define SOURCEMETA_CORE_TEST_CLITEST_H_

#include <sourcemeta/core/crypto.h>
#include <sourcemeta/core/diff.h>
#include <sourcemeta/core/gzip.h>
#include <sourcemeta/core/io.h>
#include <sourcemeta/core/numeric.h>
#include <sourcemeta/core/process.h>
#include <sourcemeta/core/regex.h>
#include <sourcemeta/core/text.h>
#include <sourcemeta/core/unicode.h>
#include <sourcemeta/core/uri.h>

#include <algorithm>    // std::sort
#include <compare>      // std::strong_ordering
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint8_t
#include <cstdlib>      // std::getenv
#include <exception>    // std::exception
#include <filesystem>   // std::filesystem::path
#include <functional>   // std::less
#include <iterator>     // std::make_move_iterator
#include <map>          // std::map
#include <optional>     // std::optional, std::nullopt
#include <set>          // std::set
#include <sstream>      // std::ostringstream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <system_error> // std::error_code
#include <thread>       // std::thread::hardware_concurrency
#include <utility>      // std::move
#include <vector>       // std::vector

namespace sourcemeta::core {

/// A failure attributed to the line of a script that caused it. The message is
/// a fixed phrase and whatever varies rides alongside it, so that whoever
/// catches this is the one that decides how the result is worded
class CLITestError : public std::exception {
public:
  CLITestError(const std::size_t line, const char *message)
      : line_{line}, message_{message} {}
  CLITestError(const std::size_t line, const char *message, std::string context)
      : line_{line}, message_{message}, context_{std::move(context)} {}
  CLITestError(const std::size_t line, const char *message, std::string context,
               const std::error_code code)
      : line_{line}, message_{message}, context_{std::move(context)},
        code_{code} {}

  CLITestError(std::size_t, std::string) = delete;
  CLITestError(std::size_t, std::string &&) = delete;
  CLITestError(std::size_t, std::string_view) = delete;

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->message_;
  }

  /// The line of the script that failed
  [[nodiscard]] auto line() const noexcept -> std::size_t {
    return this->line_;
  }

  /// Whatever the failure was about, if it was about anything in particular
  [[nodiscard]] auto context() const noexcept -> const std::string & {
    return this->context_;
  }

  /// What the operating system had to say, if it was involved
  [[nodiscard]] auto code() const noexcept -> const std::error_code & {
    return this->code_;
  }

private:
  std::size_t line_;
  const char *message_;
  std::string context_;
  std::error_code code_;
};

/// A program that a script started did not end the way the script expected
class CLITestExitError : public std::exception {
public:
  CLITestExitError(const std::size_t line, const std::int64_t expected,
                   const std::optional<int> actual, std::string observation)
      : line_{line}, expected_{expected}, actual_{actual},
        observation_{std::move(observation)} {}

  [[nodiscard]] auto what() const noexcept -> const char * override {
    return this->actual_.has_value() ? "unexpected exit code"
                                     : "the program terminated abnormally";
  }

  /// The line of the script that failed
  [[nodiscard]] auto line() const noexcept -> std::size_t {
    return this->line_;
  }

  /// The code the script was expecting
  [[nodiscard]] auto expected() const noexcept -> std::int64_t {
    return this->expected_;
  }

  /// The code the program exited with, if it exited at all
  [[nodiscard]] auto actual() const noexcept -> const std::optional<int> & {
    return this->actual_;
  }

  /// Everything the program said before it ended
  [[nodiscard]] auto observation() const noexcept -> const std::string & {
    return this->observation_;
  }

private:
  std::size_t line_;
  std::int64_t expected_;
  std::optional<int> actual_;
  std::string observation_;
};

// The names a script can expand, and what they stand for
using CLITestBindings = std::map<std::string, std::string, std::less<>>;

// A single variable form recognised inside an operand
struct CLITestVariable {
  enum class Type : std::uint8_t {
    // A doubled dollar sign, which stands for a literal one and names nothing
    Escape,
    // A reference to a binding, either bare or brace delimited
    Reference
  };

  Type type;
  std::size_t start;
  std::size_t length;
  std::string_view name;
};

inline auto clitest_is_name_start(const char character) noexcept -> bool {
  return (character >= 'A' && character <= 'Z') ||
         (character >= 'a' && character <= 'z') || character == '_';
}

inline auto clitest_is_name_character(const char character) noexcept -> bool {
  return clitest_is_name_start(character) ||
         (character >= '0' && character <= '9');
}

// The variable form beginning exactly at the given index, if any. Recognising
// the doubled dollar sign ahead of any name is what stops it ever yielding a
// reference, and consuming a name greedily is what stops a short binding from
// swallowing the front of a longer one. A dollar sign before anything that
// cannot open a name is ordinary text
inline auto clitest_variable_at(const std::string_view token,
                                const std::size_t index)
    -> std::optional<CLITestVariable> {
  if (index >= token.size() || token[index] != '$') {
    return std::nullopt;
  }

  const auto after{index + 1};
  if (after >= token.size()) {
    return std::nullopt;
  }

  if (token[after] == '$') {
    return CLITestVariable{.type = CLITestVariable::Type::Escape,
                           .start = index,
                           .length = 2,
                           .name = {}};
  }

  if (!clitest_is_name_start(token[after])) {
    return std::nullopt;
  }

  auto cursor{after + 1};
  while (cursor < token.size() && clitest_is_name_character(token[cursor])) {
    cursor += 1;
  }

  return CLITestVariable{.type = CLITestVariable::Type::Reference,
                         .start = index,
                         .length = cursor - index,
                         .name = token.substr(after, cursor - after)};
}

// The first variable form at or after the given index, if any
inline auto clitest_variable_find(const std::string_view token,
                                  const std::size_t index)
    -> std::optional<CLITestVariable> {
  for (auto cursor{token.find('$', index)}; cursor != std::string_view::npos;
       cursor = token.find('$', cursor + 1)) {
    const auto variable{clitest_variable_at(token, cursor)};
    if (variable.has_value()) {
      return variable;
    }
  }

  return std::nullopt;
}

// Normalise a path as text, never as a filesystem path. The check compares
// destinations by name, so it must reach the same answer on every platform, and
// a filesystem path would both treat a backslash as a separator on Windows and
// disagree here anyway: an empty input normalises to the current directory
// rather than to nothing, and a trailing separator is dropped
inline auto clitest_normalise_path(const std::string_view path) -> std::string {
  if (path.empty()) {
    return ".";
  }

  // POSIX leaves the meaning of exactly two leading slashes implementation
  // defined, so that one run survives while every other collapses
  std::size_t leading_slashes{0};
  if (path.starts_with("//") && !path.starts_with("///")) {
    leading_slashes = 2;
  } else if (path.starts_with("/")) {
    leading_slashes = 1;
  }

  std::vector<std::string_view> components;
  for (const auto component : sourcemeta::core::split(path, '/')) {
    if (component.empty() || component == ".") {
      continue;
    }

    // A leading upwards traversal has nothing to cancel, and neither has one
    // that follows another, so both stay as written
    if (component != ".." || (leading_slashes == 0 && components.empty()) ||
        (!components.empty() && components.back() == "..")) {
      components.push_back(component);
    } else if (!components.empty()) {
      components.pop_back();
    }
  }

  std::string result;
  result.append(leading_slashes, '/');
  for (std::size_t index{0}; index < components.size(); index += 1) {
    if (index > 0) {
      result.push_back('/');
    }

    result.append(components[index]);
  }

  if (result.empty()) {
    return ".";
  }

  return result;
}

/// The result of expanding an operand: what it came to, and the first name that
/// had no binding, if there was one
struct CLITestExpansion {
  std::string value;
  std::string_view missing;
};

// Expand every variable form in an operand. Both the check and the run share
// this walk as well as the scanner beneath it. An earlier revision of the
// language had the check reimplement expansion by hand, and four separate
// defects were traced to the two drifting apart
inline auto clitest_substitute(const std::string_view token,
                               const CLITestBindings &bindings)
    -> CLITestExpansion {
  std::string value;
  std::size_t index{0};

  while (index < token.size()) {
    const auto variable{clitest_variable_find(token, index)};
    if (!variable.has_value()) {
      value.append(token.substr(index));
      break;
    }

    value.append(token.substr(index, variable->start - index));
    if (variable->type == CLITestVariable::Type::Escape) {
      value.push_back('$');
    } else {
      const auto binding{bindings.find(variable->name)};
      if (binding == bindings.cend()) {
        return {.value = std::move(value), .missing = variable->name};
      }

      value.append(binding->second);
    }

    index = variable->start + variable->length;
  }

  return {.value = std::move(value), .missing = {}};
}

// The index just past the character class opening at the given index. A closing
// bracket is an ordinary character when it opens the class, after an optional
// negating caret, so an empty looking class holds one. An unterminated class is
// not a class at all, and is left for the engine to reject
inline auto clitest_end_of_character_class(const std::string_view pattern,
                                           const std::size_t index)
    -> std::size_t {
  auto cursor{index + 1};
  if (cursor < pattern.size() && pattern[cursor] == '^') {
    cursor += 1;
  }

  if (cursor < pattern.size() && pattern[cursor] == ']') {
    cursor += 1;
  }

  while (cursor < pattern.size()) {
    if (pattern[cursor] == '\\') {
      cursor += 2;
    } else if (pattern[cursor] == ']') {
      return cursor + 1;
    } else {
      cursor += 1;
    }
  }

  return index + 1;
}

// The first variable reference in a pattern, if it holds one. A pattern is read
// with the same scanner as an operand, but it is a regular expression rather
// than text, so three of its shapes name nothing: a backslash escapes what
// follows it, a doubled dollar sign is a pair of anchors, and a dollar sign
// inside a character class is an ordinary character
inline auto clitest_pattern_variable(const std::string_view pattern)
    -> std::optional<CLITestVariable> {
  std::size_t index{0};
  while (index < pattern.size()) {
    if (pattern[index] == '\\') {
      index += 2;
      continue;
    }

    if (pattern[index] == '[') {
      index = clitest_end_of_character_class(pattern, index);
      continue;
    }

    const auto variable{clitest_variable_at(pattern, index)};
    if (!variable.has_value()) {
      index += 1;
    } else if (variable->type == CLITestVariable::Type::Escape) {
      index = variable->start + variable->length;
    } else {
      return variable;
    }
  }

  return std::nullopt;
}

inline auto clitest_is_word_separator(const char character) noexcept -> bool {
  return character == ' ' || character == '\t' || character == '\r' ||
         character == '\n';
}

// Split a line into words the way a POSIX shell would, so that an operand
// holding spaces or regular expression punctuation is written between quotes.
// Single quotes suppress everything, including a backslash. Double quotes let a
// backslash escape only itself and the closing quote, leaving it an ordinary
// character before anything else. Outside quotes a backslash escapes whatever
// follows it. An empty pair of quotes is still a word
inline auto clitest_split_words(const std::string_view line,
                                const std::size_t number)
    -> std::vector<std::string> {
  std::vector<std::string> words;
  std::string current;
  bool started{false};
  char quote{'\0'};

  for (std::size_t index{0}; index < line.size(); index += 1) {
    const auto character{line[index]};

    if (quote == '\'') {
      if (character == '\'') {
        quote = '\0';
      } else {
        current.push_back(character);
      }

      continue;
    }

    if (quote == '"') {
      if (character == '"') {
        quote = '\0';
      } else if (character == '\\' && index + 1 < line.size() &&
                 (line[index + 1] == '\\' || line[index + 1] == '"')) {
        index += 1;
        current.push_back(line[index]);
      } else {
        current.push_back(character);
      }

      continue;
    }

    if (character == '\'' || character == '"') {
      quote = character;
      started = true;
      continue;
    }

    if (character == '\\') {
      if (index + 1 >= line.size()) {
        throw CLITestError{number, "a trailing backslash escapes nothing"};
      }

      index += 1;
      current.push_back(line[index]);
      started = true;
      continue;
    }

    if (clitest_is_word_separator(character)) {
      if (started) {
        words.push_back(std::move(current));
        current.clear();
        started = false;
      }

      continue;
    }

    current.push_back(character);
    started = true;
  }

  if (quote != '\0') {
    throw CLITestError{number, "a quote is left open at the end of the line"};
  }

  if (started) {
    words.push_back(std::move(current));
  }

  return words;
}

// One command of a script, along with the body that a file writing command
// carries and the line that the whole statement is reported against
struct CLITestStatement {
  std::size_t line{0};
  std::string keyword;
  std::vector<std::string> operands;
  std::string body;
  bool has_body{false};
};

// Read a script into its statements. Blank lines and comments carry nothing, so
// they leave no statement behind and only advance the line count
inline auto clitest_parse(const std::vector<std::string_view> &lines)
    -> std::vector<CLITestStatement> {
  std::vector<CLITestStatement> statements;
  std::size_t index{0};

  while (index < lines.size()) {
    const auto number{index + 1};
    const auto stripped{sourcemeta::core::trim(lines[index])};
    index += 1;

    if (stripped.empty() || stripped.starts_with("//")) {
      continue;
    }

    auto words{clitest_split_words(stripped, number)};
    if (words.empty()) {
      continue;
    }

    CLITestStatement statement;
    statement.line = number;
    statement.keyword = std::move(words.front());
    statement.operands.assign(std::make_move_iterator(words.begin() + 1),
                              std::make_move_iterator(words.end()));

    if (statement.keyword == "WRITE") {
      if (statement.operands.size() != 3 || statement.operands[1] != "UNTIL") {
        throw CLITestError{number, "usage: WRITE <path> UNTIL <terminator>"};
      }

      // The terminator is compared against the line exactly as it stands, so it
      // closes a body only when it sits at the very start of one. Bodies are
      // taken verbatim and never expand a variable, which is what lets a
      // fixture hold as many dollar signs as it likes
      const auto &terminator{statement.operands[2]};
      while (index < lines.size() && lines[index] != terminator) {
        statement.body.append(lines[index]);
        statement.body.push_back('\n');
        index += 1;
      }

      if (index >= lines.size()) {
        throw CLITestError{number, "this body is never closed", terminator};
      }

      index += 1;
      statement.has_body = true;
    }

    statements.push_back(std::move(statement));
  }

  return statements;
}

using CLITestTargets = std::set<std::string, std::less<>>;

// A statement that produces something, paired with where it did so and with
// the fixed phrase for it going unread
struct CLITestProduction {
  std::string target;
  std::size_t line{0};
  const char *message{nullptr};
};

// A name bound by a checksum, kept in statement order so that binding it again
// cannot hide an earlier definition that nothing ever used
struct CLITestDefinition {
  std::string name;
  std::size_t position{0};
  std::size_t line{0};
};

// Something a script produces and then never asserts on. The message is a fixed
// phrase and whatever varies rides alongside it, so that whoever reports this
// is the one that decides how the result is worded
struct CLITestProblem {
  std::size_t line{0};
  const char *message{nullptr};
  std::string context;

  // Ordered by what a reader sees rather than by where the phrase happens to
  // live in memory
  auto operator<(const CLITestProblem &other) const -> bool {
    if (this->line != other.line) {
      return this->line < other.line;
    }

    const auto ordering{std::string_view{this->message}.compare(other.message)};
    if (ordering != 0) {
      return ordering < 0;
    }

    return this->context < other.context;
  }
};

// The file an operand denotes, as a comparable name rather than as a real path.
// Nothing here touches the filesystem
inline auto clitest_target_of(const std::string_view token,
                              const CLITestBindings &bindings)
    -> std::optional<std::string> {
  // The sandbox may be named explicitly, so that a path below it and the same
  // path written bare denote one file
  std::string_view rest{token};
  if (rest == "$CWD") {
    return ".";
  }
  if (rest.starts_with("$CWD/")) {
    rest.remove_prefix(5);
  }

  // A path built from a value only known when the script runs, such as a
  // checksum, is left for the interpreter to judge rather than misjudged here
  const auto expansion{clitest_substitute(rest, bindings)};
  if (!expansion.missing.empty()) {
    return std::nullopt;
  }

  return clitest_normalise_path(expansion.value);
}

inline auto clitest_collect(CLITestTargets &destination,
                            const std::string_view token,
                            const CLITestBindings &bindings) -> void {
  const auto target{clitest_target_of(token, bindings)};
  if (target.has_value()) {
    destination.insert(target.value());
  }
}

// Whether an operand is a pattern rather than text. A pattern is never
// expanded, so a name appearing inside one is not a use of that name, however
// it is spelled. Reading it as one would let a definition that nothing ever
// expands pass this check
inline auto clitest_is_pattern_operand(const CLITestStatement &statement,
                                       const std::size_t index) -> bool {
  if (statement.keyword == "REPLACE" && statement.operands.size() == 6 &&
      statement.operands.front() == "MATCHING") {
    return index == 1;
  }
  if (statement.keyword == "DROP" && statement.operands.size() == 5 &&
      statement.operands[1] == "MATCHING") {
    return index == 2;
  }

  return false;
}

// Whether the given file is eventually compared, following the copies that may
// carry it there
inline auto clitest_asserted(
    const std::string &target, const CLITestTargets &compared,
    const std::map<std::string, CLITestTargets, std::less<>> &copies) -> bool {
  std::vector<std::string> pending{target};
  CLITestTargets seen;

  while (!pending.empty()) {
    const auto current{pending.back()};
    pending.pop_back();

    if (compared.contains(current)) {
      return true;
    }
    if (seen.contains(current)) {
      continue;
    }

    seen.insert(current);
    const auto destinations{copies.find(current)};
    if (destinations != copies.cend()) {
      pending.insert(pending.cend(), destinations->second.cbegin(),
                     destinations->second.cend());
    }
  }

  return false;
}

// Reject a script that produces something and then never asserts on it. A
// command whose result nothing reads is a command whose result nobody checked,
// and without this a script can run the tool and silently throw away everything
// it printed, passing on the exit code alone
inline auto clitest_check(const std::vector<CLITestStatement> &statements,
                          CLITestBindings bindings = {})
    -> std::vector<CLITestProblem> {
  std::vector<CLITestProduction> observations;
  std::vector<CLITestProduction> artifacts;
  std::map<std::string, CLITestTargets, std::less<>> copies;
  CLITestTargets compared;
  CLITestTargets consumed;
  std::vector<CLITestDefinition> definitions;
  std::map<std::string, std::vector<std::size_t>, std::less<>> expansions;
  std::vector<CLITestProblem> problems;

  for (std::size_t position{0}; position < statements.size(); position += 1) {
    const auto &statement{statements[position]};
    const auto &operands{statement.operands};

    for (std::size_t index{0}; index < operands.size(); index += 1) {
      if (clitest_is_pattern_operand(statement, index)) {
        // Whether a pattern holds a reference is known without running
        // anything, since a pattern is never expanded. One that does could
        // expand nothing, and an anchor is zero width, so it would match no
        // input at all and its author meant the literal form
        const auto reference{clitest_pattern_variable(operands[index])};
        if (reference.has_value()) {
          problems.push_back(
              {.line = statement.line,
               .message =
                   "a pattern cannot expand this name, so use the literal form",
               .context = std::string{reference->name}});
        }

        continue;
      }

      std::size_t cursor{0};
      while (cursor < operands[index].size()) {
        const auto variable{clitest_variable_find(operands[index], cursor)};
        if (!variable.has_value()) {
          break;
        }

        if (variable->type == CLITestVariable::Type::Reference) {
          expansions[std::string{variable->name}].push_back(position);
        }

        cursor = variable->start + variable->length;
      }
    }

    // Only the commands below read a file for its content. A filter such as a
    // replacement or a sort rewrites a file in place, which neither asserts it
    // nor hands it to anything else, so it must never count as a use
    if (statement.keyword == "RUN" && operands.size() >= 8) {
      const auto destination{
          clitest_target_of(operands[operands.size() - 3], bindings)};
      if (destination.has_value()) {
        observations.push_back(
            {.target = destination.value(),
             .line = statement.line,
             .message =
                 "nothing compares this file, so the run asserts only its "
                 "exit code"});
      }

      for (std::size_t index{0}; index + 8 < operands.size(); index += 1) {
        // A program reads its arguments before this run writes its own
        // observation, so naming the destination among them reads whatever was
        // there beforehand and cannot stand in for asserting on this run
        const auto argument{clitest_target_of(operands[index], bindings)};
        if (argument.has_value() && argument != destination) {
          consumed.insert(argument.value());
        }
      }

      clitest_collect(consumed, operands[operands.size() - 7], bindings);
      clitest_collect(consumed, operands[operands.size() - 5], bindings);
    } else if (statement.keyword == "COMPARE" && operands.size() == 3) {
      clitest_collect(compared, operands[0], bindings);
      clitest_collect(compared, operands[2], bindings);
    } else if (statement.keyword == "COPY" && operands.size() == 3) {
      const auto source{clitest_target_of(operands[0], bindings)};
      const auto destination{clitest_target_of(operands[2], bindings)};
      if (source.has_value() && destination.has_value()) {
        copies[source.value()].insert(destination.value());
      }

      if (destination.has_value()) {
        artifacts.push_back(
            {.target = destination.value(),
             .line = statement.line,
             .message = "nothing reads the file this copy produced"});
      }

      clitest_collect(consumed, operands[0], bindings);
    } else if (statement.keyword == "TREE" && operands.size() == 3) {
      const auto destination{clitest_target_of(operands[2], bindings)};
      if (destination.has_value()) {
        artifacts.push_back(
            {.target = destination.value(),
             .line = statement.line,
             .message = "nothing reads the listing this produced"});
      }

      clitest_collect(consumed, operands[0], bindings);
    } else if (statement.keyword == "COMPRESS" && operands.size() == 4) {
      const auto destination{clitest_target_of(operands[3], bindings)};
      if (destination.has_value()) {
        artifacts.push_back(
            {.target = destination.value(),
             .line = statement.line,
             .message = "nothing reads the archive this produced"});
      }

      clitest_collect(consumed, operands[1], bindings);
    } else if (statement.keyword == "CHECKSUM" && operands.size() == 4) {
      definitions.push_back(
          {.name = operands[3], .position = position, .line = statement.line});
      clitest_collect(consumed, operands[1], bindings);
    } else if (statement.keyword == "EXTRACT" && operands.size() == 5) {
      const auto destination{clitest_target_of(operands[4], bindings)};
      if (destination.has_value()) {
        artifacts.push_back({.target = destination.value(),
                             .line = statement.line,
                             .message = "nothing reads what this extracted"});
      }

      clitest_collect(consumed, operands[2], bindings);
    } else if (statement.keyword == "STAT" && operands.size() == 4) {
      const auto destination{clitest_target_of(operands[3], bindings)};
      if (destination.has_value()) {
        artifacts.push_back(
            {.target = destination.value(),
             .line = statement.line,
             .message = "nothing reads the timestamp this produced"});
      }

      clitest_collect(consumed, operands[1], bindings);
    } else if (statement.keyword == "ENV" && operands.size() == 2) {
      // Tracked so that a path spelled through a variable still resolves
      if (!operands[1].contains('$')) {
        bindings.insert_or_assign(operands[0], operands[1]);
      }
    }
  }

  for (const auto &observation : observations) {
    // An observation may legitimately flow onwards as the input of a later
    // command rather than being compared directly, as a compiled template does.
    // That later command is itself checked, so the chain still ends in an
    // assertion
    if (!clitest_asserted(observation.target, compared, copies) &&
        !consumed.contains(observation.target)) {
      problems.push_back({.line = observation.line,
                          .message = observation.message,
                          .context = observation.target});
    }
  }

  for (const auto &artifact : artifacts) {
    if (!consumed.contains(artifact.target) &&
        !compared.contains(artifact.target)) {
      problems.push_back({.line = artifact.line,
                          .message = artifact.message,
                          .context = artifact.target});
    }
  }

  for (std::size_t index{0}; index < definitions.size(); index += 1) {
    // Only a use before the name is bound again can justify this definition.
    // Definitions are in statement order, so the first later one is the nearest
    auto rebound{statements.size()};
    for (std::size_t later{index + 1}; later < definitions.size(); later += 1) {
      if (definitions[later].name == definitions[index].name) {
        rebound = definitions[later].position;
        break;
      }
    }

    auto justified{false};
    const auto uses{expansions.find(definitions[index].name)};
    if (uses != expansions.cend()) {
      for (const auto use : uses->second) {
        if (definitions[index].position < use && use < rebound) {
          justified = true;
          break;
        }
      }
    }

    if (!justified) {
      problems.push_back({.line = definitions[index].line,
                          .message = "nothing expands this variable",
                          .context = definitions[index].name});
    }
  }

  std::sort(problems.begin(), problems.end());
  return problems;
}

// The sandbox a script runs against, along with everything it can expand
struct CLITestEnvironment {
  std::filesystem::path sandbox;
  CLITestBindings bindings;
};

inline auto clitest_is_reserved(const std::string_view name) noexcept -> bool {
  return name == "CWD" || name == "CWD_URI" || name == "CORES";
}

inline auto clitest_environment(const std::filesystem::path &sandbox,
                                CLITestBindings bindings)
    -> CLITestEnvironment {
  // The expectations a script carries are written with forward slashes, which
  // is the generic form of a path rather than the native one
  bindings.insert_or_assign("CWD", sandbox.generic_string());
  // Not merely the sandbox with a scheme prepended, which would read a Windows
  // drive letter as the authority of the URI
  bindings.insert_or_assign("CWD_URI", URI::from_path(sandbox).recompose());
  // Spelled exactly as the program under test spells its own default
  // parallelism, so that the two agree by construction rather than by
  // inspection. The standard library is allowed not to know the answer
  bindings.insert_or_assign(
      "CORES", std::to_string(std::max(static_cast<std::size_t>(
                                           std::thread::hardware_concurrency()),
                                       static_cast<std::size_t>(1))));
  return {.sandbox = sandbox, .bindings = std::move(bindings)};
}

inline auto clitest_expand(const CLITestEnvironment &environment,
                           const std::string_view token, const std::size_t line)
    -> std::string {
  auto expansion{clitest_substitute(token, environment.bindings)};
  if (!expansion.missing.empty()) {
    throw CLITestError{line, "undefined variable",
                       std::string{expansion.missing}};
  }

  return std::move(expansion.value);
}

// Paths are relative to the sandbox unless absolute
inline auto clitest_resolve(const CLITestEnvironment &environment,
                            const std::string_view token,
                            const std::size_t line) -> std::filesystem::path {
  const auto expanded{clitest_expand(environment, token, line)};
  // A leading separator names an absolute path here on every platform, where a
  // filesystem path would call it relative on Windows for want of a drive
  if (expanded.starts_with('/')) {
    return {expanded};
  }

  const std::filesystem::path path{expanded};
  if (path.is_absolute()) {
    return path;
  }

  return environment.sandbox / path;
}

/// The lines of a file, without their terminators, and whether the last of them
/// carried one
struct CLITestLines {
  std::vector<std::string_view> values;
  bool terminated{false};
};

// A terminator closing the last line does not open an empty one after it
inline auto clitest_lines(const std::string_view content) -> CLITestLines {
  auto values{sourcemeta::core::split(content, '\n')};
  const auto terminated{content.ends_with('\n')};
  if (terminated || content.empty()) {
    values.pop_back();
  }

  return {.values = std::move(values), .terminated = terminated};
}

inline auto clitest_join(const std::vector<std::string_view> &lines,
                         const bool terminated = true) -> std::string {
  auto result{sourcemeta::core::join(lines, "\n")};
  if (terminated && !lines.empty()) {
    result.push_back('\n');
  }

  return result;
}

// A stream contributes nothing at all when it is empty, not even a marker. Note
// the emptiness is judged before the trailing terminator is dropped, so a
// stream of one terminator is one empty line rather than nothing
inline auto clitest_observe_stream(std::string &result,
                                   const std::string_view marker,
                                   const std::string_view stream) -> void {
  if (stream.empty()) {
    return;
  }

  auto text{sourcemeta::core::replace(sourcemeta::core::to_valid_utf8(stream),
                                      "\r\n", "\n")};
  if (text.ends_with('\n')) {
    text.pop_back();
  }

  for (const auto line : sourcemeta::core::split(text, '\n')) {
    result.append(marker);
    if (!line.empty()) {
      result.push_back(' ');
      result.append(line);
    }

    result.push_back('\n');
  }
}

// Both streams of a run, captured into one file. Standard output in full first
// and then standard error, never interleaved, so that an observation is
// reproducible rather than a race
inline auto clitest_observe(const std::string_view standard_output,
                            const std::string_view standard_error)
    -> std::string {
  std::string result;
  clitest_observe_stream(result, "1>", standard_output);
  clitest_observe_stream(result, "2>", standard_error);
  return result;
}

/// One command being carried out: the statement, and everything it runs
/// against. Operands are reached by index, since threading the statement and
/// the line it sits on through every call is otherwise most of what a command
/// would have to say
struct CLITestCommand {
  const CLITestStatement &statement;
  CLITestEnvironment &environment;
  const std::string &binary;

  [[nodiscard]] auto size() const -> std::size_t {
    return this->statement.operands.size();
  }

  [[nodiscard]] auto line() const -> std::size_t {
    return this->statement.line;
  }

  [[nodiscard]] auto at(const std::size_t index) const -> const std::string & {
    return this->statement.operands[index];
  }

  [[nodiscard]] auto expand(const std::size_t index) const -> std::string {
    return clitest_expand(this->environment, this->at(index), this->line());
  }

  [[nodiscard]] auto resolve(const std::size_t index) const
      -> std::filesystem::path {
    return clitest_resolve(this->environment, this->at(index), this->line());
  }

  // Failures name the file the way the script named it. The sandbox is a
  // different directory on every run and is gone by the time anyone reads the
  // message, so quoting it in full would be noise nobody can act on
  [[nodiscard]] auto read(const std::size_t index) const -> std::string {
    try {
      auto stream{sourcemeta::core::read_file(this->resolve(index))};
      return sourcemeta::core::read_to_string(stream);
    } catch (const IOFileNotFoundError &) {
      this->fail("no such file", this->expand(index));
    } catch (const IOIsADirectoryError &) {
      this->fail("is a directory", this->expand(index));
    } catch (const IOFilePermissionError &) {
      this->fail("cannot be read", this->expand(index));
    }
  }

  auto write(const std::size_t index, const std::string_view contents) const
      -> void {
    sourcemeta::core::write_file(this->resolve(index), contents);
  }

  [[noreturn]] auto usage(const std::string_view form) const -> void {
    throw CLITestError{this->line(), "usage", std::string{form}};
  }

  [[noreturn]] auto fail(const char *message) const -> void {
    throw CLITestError{this->line(), message};
  }

  [[noreturn]] auto fail(const char *message, std::string context) const
      -> void {
    throw CLITestError{this->line(), message, std::move(context)};
  }

  [[noreturn]] auto fail(const char *message, std::string context,
                         const std::error_code code) const -> void {
    throw CLITestError{this->line(), message, std::move(context), code};
  }
};

inline auto clitest_command_write(const CLITestCommand &command) -> void {
  command.write(0, command.statement.body);
}

inline auto clitest_command_env(const CLITestCommand &command) -> void {
  if (command.size() != 2) {
    command.usage("ENV <name> <value>");
  } else if (clitest_is_reserved(command.at(0))) {
    command.fail("this name is reserved and cannot be set", command.at(0));
  }

  command.environment.bindings.insert_or_assign(command.at(0),
                                                command.expand(1));
}

// Read a variable of the environment this runner was started from. The standard
// way of doing so is deprecated by one toolchain over a hazard that does not
// arise here, as the value is copied before anything else can run
inline auto clitest_inherited(const char *const name)
    -> std::optional<std::string> {
#if defined(_MSC_VER)
  char *value{nullptr};
  std::size_t size{0};
  if (::_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
    return std::nullopt;
  }

  std::string result{value};
  std::free(value);
  return result;
#else
  const char *const value{std::getenv(name)};
  if (value == nullptr) {
    return std::nullopt;
  }

  return std::string{value};
#endif
}

inline auto clitest_command_run(const CLITestCommand &command) -> void {
  if (command.size() < 8) {
    command.fail("RUN needs STDIN, IN, INTO and EXPECTING");
  }

  // Read from the end, so that an argument may hold the literal words that make
  // up the trailer
  const auto trailer{command.size() - 8};
  if (command.at(trailer) != "STDIN" || command.at(trailer + 2) != "IN" ||
      command.at(trailer + 4) != "INTO" ||
      command.at(trailer + 6) != "EXPECTING") {
    command.usage("RUN [arguments...] STDIN <file> IN <dir> INTO <dest> "
                  "EXPECTING <code>");
  }

  std::vector<std::string> arguments;
  arguments.reserve(trailer);
  for (std::size_t index{0}; index < trailer; index += 1) {
    arguments.push_back(command.expand(index));
  }

  std::vector<std::string_view> argument_views;
  argument_views.reserve(arguments.size());
  for (const auto &argument : arguments) {
    argument_views.emplace_back(argument);
  }

  const auto code{command.expand(trailer + 7)};
  const auto expected_code{sourcemeta::core::to_int64_t(code)};
  if (!expected_code.has_value()) {
    command.fail("not an exit code", code);
  }

  // The null device means empty input on every platform, including the ones
  // that have no such file to open
  std::string standard_input;
  if (command.expand(trailer + 1) != "/dev/null") {
    standard_input = command.read(trailer + 1);
  }

  // The environment of a program replaces rather than extends the one it was
  // started from, so everything it cannot do without has to be named here
  std::map<std::string_view, std::string_view> child;
  for (const auto &binding : command.environment.bindings) {
    if (binding.first != "CWD") {
      child.emplace(binding.first, binding.second);
    }
  }

  const auto path_value{clitest_inherited("PATH").value_or(std::string{})};
  child.insert_or_assign("PATH", path_value);

#if defined(_WIN32)
  // Reserved up front, as the map holds views into these and a reallocation
  // would move a short string along with the storage it lives inside
  constexpr std::array<const char *, 3> ESSENTIALS{
      {"SYSTEMROOT", "COMSPEC", "TEMP"}};
  std::vector<std::string> essentials;
  essentials.reserve(ESSENTIALS.size());
  for (const auto name : ESSENTIALS) {
    const auto value{clitest_inherited(name)};
    if (value.has_value()) {
      essentials.push_back(value.value());
      child.insert_or_assign(name, essentials.back());
    }
  }
#endif

  // A program that cannot be started at all is a failure of the script rather
  // than of the runner, so it is reported against the line that asked for it
  ProcessOutput output;
  try {
    output = sourcemeta::core::spawn_and_capture(
        command.binary, argument_views,
        {.directory = command.resolve(trailer + 3),
         .environment = child,
         .standard_input = standard_input});
  } catch (const ProcessProgramNotFoundError &error) {
    command.fail("cannot find the program to run",
                 std::string{error.program()});
  }

  const auto observation{
      clitest_observe(output.standard_output, output.standard_error)};
  // Written before the code is judged, so that a run which fails still leaves
  // behind everything the program said
  command.write(trailer + 5, observation);

  if (!output.exit_code.has_value() ||
      output.exit_code.value() != expected_code.value()) {
    throw CLITestExitError{command.line(), expected_code.value(),
                           output.exit_code, observation};
  }
}

inline auto clitest_command_compare(const CLITestCommand &command) -> void {
  if (command.size() != 3 || command.at(1) != "AGAINST") {
    command.usage("COMPARE <actual> AGAINST <expected>");
  }

  const auto actual{command.read(0)};
  const auto expected{command.read(2)};
  if (actual == expected) {
    return;
  }

  const auto difference{sourcemeta::core::diff(
      expected, actual, Diff::Mode::Line, Diff::Algorithm::Myers)};
  std::ostringstream stream;
  sourcemeta::core::stringify(
      difference, stream, Diff::Format::Unified,
      {.original_label = command.at(2), .modified_label = command.at(0)});
  command.fail("the files differ", stream.str());
}

inline auto clitest_command_replace(const CLITestCommand &command) -> void {
  // The two forms are told apart by length, so that replacing the literal text
  // that names the pattern form stays possible
  const auto matching{command.size() == 6 && command.at(0) == "MATCHING" &&
                      command.at(2) == "WITH" && command.at(4) == "IN"};
  if (!matching && !(command.size() == 5 && command.at(1) == "WITH" &&
                     command.at(3) == "IN")) {
    command.usage("REPLACE [MATCHING] <pattern> WITH <replacement> IN <file>");
  }

  const auto target{matching ? 5U : 4U};
  const auto content{command.read(target)};
  const auto replacement{command.expand(matching ? 3U : 2U)};

  if (!matching) {
    command.write(target, sourcemeta::core::replace(content, command.expand(0),
                                                    replacement));
    return;
  }

  // Compiled without the match optimisation, as the result has to rewrite its
  // subject rather than only answer whether it matched
  const auto regex{sourcemeta::core::to_regex(command.at(1),
                                              RegexDialect::Permissive, false)};
  if (!regex.has_value()) {
    command.fail("bad pattern", command.at(1));
  }

  command.write(target, sourcemeta::core::replace_all(regex.value(), content,
                                                      replacement));
}

inline auto clitest_command_drop(const CLITestCommand &command) -> void {
  if (command.size() != 5 || command.at(0) != "LINES" ||
      command.at(3) != "IN" ||
      (command.at(1) != "CONTAINING" && command.at(1) != "MATCHING")) {
    command.usage("DROP LINES CONTAINING|MATCHING <pattern> IN <file>");
  }

  const auto content{command.read(4)};
  auto lines{clitest_lines(content)};

  if (command.at(1) == "CONTAINING") {
    const auto text{command.expand(2)};
    std::erase_if(lines.values,
                  [&text](const std::string_view candidate) -> bool {
                    return candidate.contains(text);
                  });
  } else {
    const auto regex{sourcemeta::core::to_regex(command.at(2))};
    if (!regex.has_value()) {
      command.fail("bad pattern", command.at(2));
    }

    // The terminator is kept out of the subject, so that an anchor in a pattern
    // means the end of the line an author was looking at
    std::erase_if(lines.values,
                  [&regex](const std::string_view candidate) -> bool {
                    return sourcemeta::core::matches(regex.value(), candidate);
                  });
  }

  command.write(4, clitest_join(lines.values, lines.terminated));
}

inline auto clitest_command_extract(const CLITestCommand &command) -> void {
  if (command.size() != 5 || command.at(0) != "STDOUT" ||
      command.at(1) != "FROM" || command.at(3) != "INTO") {
    command.usage("EXTRACT STDOUT FROM <observation> INTO <destination>");
  }

  const auto content{command.read(2)};
  std::vector<std::string_view> extracted;
  for (const auto candidate : clitest_lines(content).values) {
    if (candidate == "1>") {
      extracted.emplace_back();
    } else if (candidate.starts_with("1> ")) {
      extracted.push_back(candidate.substr(3));
    }
  }

  command.write(4, clitest_join(extracted));
}

inline auto clitest_command_sort(const CLITestCommand &command) -> void {
  if (command.size() != 1) {
    command.usage("SORT <file>");
  }

  const auto content{command.read(0)};
  auto lines{clitest_lines(content)};
  std::ranges::sort(lines.values);
  // A filter that rewrites a file in place changes its order, not its shape, so
  // a file that ended without a terminator still does
  command.write(0, clitest_join(lines.values, lines.terminated));
}

inline auto clitest_command_copy(const CLITestCommand &command) -> void {
  if (command.size() != 3 || command.at(1) != "TO") {
    command.usage("COPY <source> TO <destination>");
  }

  const auto destination{command.resolve(2)};
  if (destination.has_parent_path()) {
    std::filesystem::create_directories(destination.parent_path());
  }

  std::error_code error;
  std::filesystem::copy_file(command.resolve(0), destination,
                             std::filesystem::copy_options::overwrite_existing,
                             error);
  if (error) {
    command.fail("cannot copy this file", command.expand(0), error);
  }
}

inline auto clitest_command_tree(const CLITestCommand &command) -> void {
  if (command.size() != 3 || command.at(1) != "INTO") {
    command.usage("TREE <directory> INTO <destination>");
  }

  const auto root{command.resolve(0)};
  std::vector<std::string> entries;
  std::error_code error;
  // Directories are listed alongside files, and a symbolic link is an entry
  // rather than somewhere to descend into
  for (const auto &entry : std::filesystem::recursive_directory_iterator{
           root, std::filesystem::directory_options::none, error}) {
    entries.push_back(
        "./" +
        std::filesystem::relative(entry.path(), root, error).generic_string());
  }

  if (error) {
    command.fail("cannot list this directory", command.expand(0), error);
  }

  // Ordered by code point, which for this encoding is the order of the bytes
  std::ranges::sort(entries);

  std::string result;
  for (const auto &entry : entries) {
    result.append(entry);
    result.push_back('\n');
  }

  command.write(2, result);
}

inline auto clitest_command_compress(const CLITestCommand &command) -> void {
  if (command.size() != 4 || command.at(0) != "GZIP" ||
      command.at(2) != "INTO") {
    command.usage("COMPRESS GZIP <source> INTO <destination>");
  }

  const auto content{command.read(1)};
  command.write(3, sourcemeta::core::gzip(
                       reinterpret_cast<const std::uint8_t *>(content.data()),
                       content.size()));
}

inline auto clitest_command_checksum(const CLITestCommand &command) -> void {
  if (command.size() != 4 || command.at(0) != "SHA256" ||
      command.at(2) != "AS") {
    command.usage("CHECKSUM SHA256 <file> AS <variable>");
  } else if (clitest_is_reserved(command.at(3))) {
    command.fail("this name is reserved and cannot be set", command.at(3));
  }

  command.environment.bindings.insert_or_assign(
      command.at(3), sourcemeta::core::sha256(command.read(1)));
}

inline auto clitest_command_stat(const CLITestCommand &command) -> void {
  if (command.size() != 4 || command.at(0) != "MTIME" ||
      command.at(2) != "INTO") {
    command.usage("STAT MTIME <file> INTO <destination>");
  }

  std::error_code error;
  const auto stamp{std::filesystem::last_write_time(command.resolve(1), error)};
  if (error) {
    command.fail("cannot read the modification time", command.expand(1), error);
  }

  // Only ever compared against another reading of the same file, so any stable
  // representation of the instant will do
  command.write(3, std::to_string(static_cast<std::int64_t>(
                       stamp.time_since_epoch().count())) +
                       "\n");
}

inline auto clitest_command_remove(const CLITestCommand &command) -> void {
  if (command.size() != 1) {
    command.usage("REMOVE <path>");
  }

  // Silent when the target was never there to begin with, but a target that is
  // there and survives anyway leaves the script running against a stale file
  std::error_code error;
  std::filesystem::remove_all(command.resolve(0), error);
  if (error) {
    command.fail("cannot remove this path", command.expand(0), error);
  }
}

inline auto clitest_command_make_directory(const CLITestCommand &command)
    -> void {
  if (command.size() != 2 || command.at(0) != "DIRECTORY") {
    command.usage("MAKE DIRECTORY <path>");
  }

  std::filesystem::create_directories(command.resolve(1));
}

inline auto clitest_interpret(const std::vector<CLITestStatement> &statements,
                              const std::string &binary,
                              CLITestEnvironment &environment) -> void {
  for (const auto &statement : statements) {
    const CLITestCommand command{
        .statement = statement, .environment = environment, .binary = binary};
    const auto &keyword{statement.keyword};

    if (keyword == "WRITE") {
      clitest_command_write(command);
    } else if (keyword == "RUN") {
      clitest_command_run(command);
    } else if (keyword == "COMPARE") {
      clitest_command_compare(command);
    } else if (keyword == "ENV") {
      clitest_command_env(command);
    } else if (keyword == "REPLACE") {
      clitest_command_replace(command);
    } else if (keyword == "DROP") {
      clitest_command_drop(command);
    } else if (keyword == "TREE") {
      clitest_command_tree(command);
    } else if (keyword == "COPY") {
      clitest_command_copy(command);
    } else if (keyword == "COMPRESS") {
      clitest_command_compress(command);
    } else if (keyword == "CHECKSUM") {
      clitest_command_checksum(command);
    } else if (keyword == "EXTRACT") {
      clitest_command_extract(command);
    } else if (keyword == "SORT") {
      clitest_command_sort(command);
    } else if (keyword == "STAT") {
      clitest_command_stat(command);
    } else if (keyword == "REMOVE") {
      clitest_command_remove(command);
    } else if (keyword == "MAKE") {
      clitest_command_make_directory(command);
    } else {
      throw CLITestError{statement.line, "unknown command", keyword};
    }
  }
}

} // namespace sourcemeta::core

#endif
