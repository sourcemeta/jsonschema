#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

"$1" fmt "$TMP/does_not_exist.json" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1
