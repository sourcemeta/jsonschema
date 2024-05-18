#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
# shellcheck disable=SC2317
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" fmt "$TMP/does_not_exist.json" && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
else
  echo "PASS" 1>&2
  exit 0
fi
