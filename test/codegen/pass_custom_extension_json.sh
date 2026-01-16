#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" codegen "$TMP/schema.custom" --target typescript > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
export type Schema = string;
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
