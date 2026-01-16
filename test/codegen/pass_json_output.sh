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

"$1" codegen "$TMP/schema.json" --target typescript --json > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
{
  "code": "export type Schema = string;\n"
}
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
