#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "enum": [ "foo" ],
  "contentMediaType": "application/json",
  "x-lint-exclude": [
    "enum_with_type",
    "content_media_type_without_encoding",
    "enum_to_const"
  ]
}
EOF

"$1" lint "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
