#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/openapi.json"
{
  "openapi": "3.0.0",
  "info": {
    "title": "Test API",
    "version": "1.0.0"
  },
  "components": {
    "schemas": {
      "User": {
        "$schema": "https://json-schema.org/draft/2020-12/schema",
        "type": "object"
      }
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": "John", "age": 30 }
EOF

"$1" validate "$TMP/openapi.json" "$TMP/instance.json" \
  --path "/components/schemas/User"

# Verbose run
"$1" validate "$TMP/openapi.json" "$TMP/instance.json" \
  --path "/components/schemas/User" --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_verbose.txt"
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/openapi.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected_verbose.txt"

# JSON run
"$1" validate "$TMP/openapi.json" "$TMP/instance.json" \
  --path "/components/schemas/User" --json > "$TMP/stdout.txt"

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "valid": true
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected_json.txt"
