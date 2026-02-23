#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    },
    "age": {
      "type": "integer"
    }
  }
}
EOF

mkdir "$TMP/instances"

cat << 'EOF' > "$TMP/instances/instance_1.json"
{ "name": "Alice", "age": 30 }
EOF

cat << 'EOF' > "$TMP/instances/instance_2.json"
{ "name": "Bob", "age": "invalid" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instances" --json > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/instances/instance_1.json
{
  "valid": true
}
$(realpath "$TMP")/instances/instance_2.json
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/age/type",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties/age/type",
      "instanceLocation": "/age",
      "instancePosition": [ 1, 18, 1, 33 ],
      "error": "The value was expected to be of type integer but it was of type string"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 35 ],
      "error": "The object value was expected to validate against the defined properties subschemas"
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
