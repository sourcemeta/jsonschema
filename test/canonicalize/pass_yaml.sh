#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
type: string
EOF

"$1" canonicalize "$TMP/schema.yaml" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "minLength": 0
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
