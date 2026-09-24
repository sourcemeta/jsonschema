#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/lf.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
type: string
title: Hello World
EOF
awk '{ printf "%s\r\n", $0 }' "$TMP/lf.yaml" > "$TMP/schema.yaml"

"$1" fmt "$TMP/schema.yaml" > "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected_output.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected_lf.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
title: Hello World
type: string
EOF
awk '{ printf "%s\r\n", $0 }' "$TMP/expected_lf.yaml" > "$TMP/expected.yaml"

diff "$TMP/schema.yaml" "$TMP/expected.yaml"
