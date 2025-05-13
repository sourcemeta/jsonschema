#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" --version 1> "$TMP/stdout" 2> "$TMP/stderr"

if ! [ -s "$TMP/stdout" ]
then
  echo "FAIL: Did not produce output to stdout" 1>&2
  exit 1
fi

cat "$TMP/stdout"
grep -q '^[0-9]\+\.[0-9]\+\.[0-9]\+$' "$TMP/stdout" || \
  (echo "The output does not look like a valid version" 1>&2 && exit 1)

if [ -s "$TMP/stderr" ]
then
  echo "FAIL: Produced output to stderr" 1>&2
  cat "$TMP/stderr.txt"
  exit 1
fi
