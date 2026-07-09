#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
true
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" validate --default-dialect "https://json-schema.org/draft/2020-12/schema" \
  "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
