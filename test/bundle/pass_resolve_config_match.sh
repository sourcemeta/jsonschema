#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "path": "./schemas",
  "resolve": {
    "https://example.com/my-external-schema.json": "../dependency.json"
  }
}
EOF

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/test.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "$ref": "https://example.com/my-external-schema.json"
}
EOF

cat << 'EOF' > "$TMP/dependency.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schemas/test.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "$ref": "https://example.com/my-external-schema.json",
  "$defs": {
    "https://example.com/my-external-schema.json": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/my-external-schema.json",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
