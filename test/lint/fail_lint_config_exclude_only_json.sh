#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "lint": {
    "exclude": [ "enum_with_type" ]
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
  "enum": [ "foo", "bar" ]
}
EOF

"$1" lint --only enum_with_type --json "$TMP/schema.json" \
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
      "id": "enum_with_type",
      "message": "Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": [ "type" ],
      "position": [ 6, 3, 6, 18 ]
    },
    {
      "path": "$(realpath "$TMP")/schema.json",
      "id": "enum_with_type",
      "message": "Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": [ "enum" ],
      "position": [ 7, 3, 7, 26 ]
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
