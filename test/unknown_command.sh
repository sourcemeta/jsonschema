#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" unknown_command 1> "$TMP/stdout" 2> "$TMP/stderr" || exit_code=$?

if [ -z "${exit_code:-}" ] || [ "$exit_code" -eq 0 ]; then
  echo "FAIL: Unknown command did not return exit code 1" 1>&2
  exit 1
fi

if [ -s "$TMP/stdout" ]; then
  echo "FAIL: Unexpected output to stdout for unknown command" 1>&2
  exit 1
fi

if ! [ -s "$TMP/stderr" ]; then
  echo "FAIL: No error message produced to stderr for unknown command" 1>&2
  exit 1
fi

echo "PASS" 1>&2
