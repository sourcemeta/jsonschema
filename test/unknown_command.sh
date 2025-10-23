#!/bin/sh
set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" unknown_command 1> "$TMP/stdout" 2> "$TMP/stderr" && CODE="$?" || CODE="$?"
if [ "$CODE" != "1" ]
then
  echo "FAIL: Unknown command did not return exit code 1" 1>&2
  exit 1
fi
if [ -s "$TMP/stdout" ]
then
  echo "FAIL: Unexpected output to stdout for unknown command" 1>&2
  exit 1
fi
if ! [ -s "$TMP/stderr" ]
then
  echo "FAIL: No error message produced to stderr for unknown command" 1>&2
  exit 1
fi

cat << EOF > "$TMP/expected.txt"
error: Unknown command
  at command unknown_command

Run the \`help\` command for usage information
EOF

diff "$TMP/stderr" "$TMP/expected.txt"

echo "PASS" 1>&2