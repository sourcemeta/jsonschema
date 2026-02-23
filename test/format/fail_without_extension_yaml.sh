#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema"
$schema: https://json-schema.org/draft/2020-12/schema
description: Test schema
additionalProperties: false
title: Hello World
properties:
  foo: {}
  bar: {}
EOF

"$1" fmt "$TMP/schema" 2>"$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Not supported
test "$EXIT_CODE" = "3" || exit 1

cat << EOF > "$TMP/expected.txt"
error: This command does not support YAML input files yet
  at file path $(realpath "$TMP")/schema
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
