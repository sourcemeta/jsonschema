#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

echo '"foo"' | "$1" validate "$TMP/schema.json" - >"$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
