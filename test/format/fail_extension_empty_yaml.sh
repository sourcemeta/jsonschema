#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema1"
$schema: https://json-schema.org/draft/2020-12/schema
description: Test schema
additionalProperties: false
title: Hello World
EOF

"$1" fmt "$TMP/schemas" --extension '' 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.txt"
warning: Matching files with no extension
error: This command does not support YAML input files yet
  at file path $(realpath "$TMP")/schemas/schema1
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
