#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [
      "enum_with_type",
      "content_media_type_without_encoding",
      "enum_to_const"
    ]
  }
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ "foo" ],
  "type": "string",
  "enum": [ "foo" ],
  "contentMediaType": "application/json"
}
EOF

cd "$TMP"
"$1" lint --verbose "$TMP/schema.json" > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Disabling rule from configuration: content_media_type_without_encoding
Disabling rule from configuration: enum_to_const
Disabling rule from configuration: enum_with_type
Using extension: .json
Using extension: .yaml
Using extension: .yml
Linting: $(realpath "$TMP")/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
