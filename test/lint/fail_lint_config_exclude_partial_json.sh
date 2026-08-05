#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [ "enum_with_type", "enum_to_const" ]
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

"$1" lint --json "$TMP/schema.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Lint violation
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
{
  "valid": false,
  "health": 0,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "content_media_type_without_encoding",
      "message": "The \`contentMediaType\` keyword is meaningless without the presence of the \`contentEncoding\` keyword",
      "description": null,
      "schemaLocation": [ "contentMediaType" ],
      "position": [ 8, 3, 8, 40 ]
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
