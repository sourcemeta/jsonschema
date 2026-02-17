#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas"
mkdir -p "$TMP/schemas/ignored"

cat << 'EOF' > "$TMP/schemas/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test"
}
EOF

cat << 'EOF' > "$TMP/schemas/ignored/unformatted.json"
{"$schema":"https://json-schema.org/draft/2020-12/schema","additionalProperties":false,"title":"Bad","properties":{"foo":{}}}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./schemas",
  "ignore": [
    "./ignored"
  ]
}
EOF

cd "$TMP"
"$1" fmt --check > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
