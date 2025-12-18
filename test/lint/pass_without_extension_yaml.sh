#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema"
$schema: http://json-schema.org/draft-06/schema#
title: Test schema
description: A test schema
type: string
examples:
  - foo
EOF

"$1" lint "$TMP/schema" > "$TMP/stdout.txt" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected_stdout.txt"
EOF

cat << 'EOF' > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stdout.txt" "$TMP/expected_stdout.txt"
diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"
