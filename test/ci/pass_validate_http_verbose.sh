#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "allOf": [
    { "$ref": "https://schemas.sourcemeta.com/jsonschema/draft4/schema.json" }
  ]
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "type": "string" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --http --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
Resolving over HTTP: https://schemas.sourcemeta.com/jsonschema/draft4/schema.json
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
