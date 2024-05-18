#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" help 1> "$TMP/stdout" 2> "$TMP/stderr"

if ! [ -s "$TMP/stdout" ]
then
  echo "FAIL: Did not produce output to stdout" 1>&2
  exit 1
fi

if [ -s "$TMP/stderr" ]
then
  echo "FAIL: Produced output to stderr" 1>&2
  exit 1
fi

echo "PASS" 1>&2
