#!/bin/sh

set -o errexit
set -o nounset

VERSION="$("$1" version)"

if echo "$VERSION" | grep --quiet --extended-regexp '^[0-9]+\.[0-9]+\.[0-9]+$'
then
  echo "PASS" 1>&2
else
  echo "GOT: $VERSION" 1>&2
  echo "FAIL" 1>&2
  exit 1
fi
