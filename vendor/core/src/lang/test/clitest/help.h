#ifndef SOURCEMETA_CORE_TEST_CLITEST_HELP_H_
#define SOURCEMETA_CORE_TEST_CLITEST_HELP_H_

#include <string_view> // std::string_view

namespace sourcemeta::core {

inline constexpr std::string_view CLITEST_HELP{
    R"HELP(clitest - an interpreter for the CLI test language

USAGE
  clitest <script>... -b <program> [-e NAME=VALUE]...
  clitest <script>... -c
  clitest -h

OPTIONS
  -b, --binary <program>    The program that RUN starts
  -e, --environment K=V     Bind a variable, may be given more than once
  -c, --check               Check each script without running it
  -h, --help                Show this text

Each script runs in a fresh temporary directory of its own. Results go to
standard output as TAP version 14, one test point per script, named by its path.
The exit code is 1 if any script failed.

COMMANDS

A script is one command per line. Blank lines and lines starting with // are
ignored. Lines are split with POSIX shell quoting.

  WRITE <path> UNTIL <terminator>     the following lines, up to a line equal
                                      to <terminator>, become the file
  RUN [argument...] STDIN <path> IN <directory> INTO <path> EXPECTING <code>
  COMPARE <actual> AGAINST <expected>
  REPLACE <text> WITH <replacement> IN <path>
  REPLACE MATCHING <pattern> WITH <replacement> IN <path>
  DROP LINES CONTAINING <text> IN <path>
  DROP LINES MATCHING <pattern> IN <path>
  EXTRACT STDOUT FROM <observation> INTO <destination>
  SORT <path>
  COPY <source> TO <destination>
  TREE <directory> INTO <destination>
  COMPRESS GZIP <source> INTO <destination>
  CHECKSUM SHA256 <path> AS <variable>
  STAT MTIME <path> INTO <destination>
  ENV <name> <value>
  MAKE DIRECTORY <path>
  REMOVE <path>

RUN is read from the end: the last eight tokens are its trailer and everything
before them is passed to the program, so an argument may hold the words STDIN,
IN, INTO or EXPECTING. STDIN /dev/null is empty input on every platform. TREE on
the root lists the harness's own scratch files, so list a subdirectory instead.

A carriage return before a line feed is dropped as the script is read, so a
checkout that spells line endings either way runs the same, and a WRITE body
cannot hold one. Beyond that a line ends at a line feed and nothing else. A
WRITE terminator is compared against the whole line, so it closes a body only
at the very start of one.

PATHS AND VARIABLES

Paths are relative to the temporary directory unless absolute. $CWD names that
directory, forward slashed and fully resolved. $CWD_URI is the same directory as
a file:// URI. $CORES is the online processor count. These three are reserved.

A reference is written $NAME, matched greedily, and $$ is one literal dollar
sign. Anything else after a dollar sign, ${NAME} included, is ordinary text.
Operands are expanded, WRITE bodies are not, so a fixture may hold $schema, $id
and $defs unescaped. ENV and CHECKSUM bind more names, and every variable except
$CWD reaches the process that RUN starts.

LITERALS AND PATTERNS

A filter matches literal text. MATCHING opts into an ECMA-262 regular expression
with PCRE2 extensions, taken verbatim and never expanding a variable. Note that
a dot matches a line terminator, and that $ anchors the absolute end, which in
DROP is the end of the line since matching is per line.

In a pattern $ is an anchor, so a literal dollar sign is written \$ or [$], and
an unescaped $ before a name is rejected. A replacement is literal in both forms
and is expanded, so it cannot reference a capture group.

OBSERVATIONS

RUN captures both streams into one file, standard output in full first and then
standard error. Each line carries its stream marker, an empty line becoming a
bare marker. A carriage return before a line feed is dropped, one trailing
terminator is dropped, an empty stream contributes nothing at all, and
ill-formed text is replaced. The file is written before the exit code is judged.

THE VACUITY CHECK

A script that produces something it never asserts on is rejected before it runs.

  - An observation must reach a COMPARE, or feed a later command that is itself
    checked. This is transitive through COPY.
  - A COPY, TREE, COMPRESS, EXTRACT or STAT destination must be read.
  - A CHECKSUM variable must be expanded before that name is bound again.
  - Only commands that read a file for its content count, so an in-place filter
    such as REPLACE or SORT does not.
  - A name inside a pattern is not a use of that name.
  - A path built from a value known only at run time is left to be judged then.
)HELP"};

} // namespace sourcemeta::core

#endif
