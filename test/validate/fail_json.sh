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
  "type": "object",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/output.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/foo/type",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties/foo/type",
      "instanceLocation": "/foo",
      "instancePosition": [ 1, 3, 1, 10 ],
      "error": "The value was expected to be of type string but it was of type integer"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 12 ],
      "error": "The object value was expected to validate against the single defined property subschema"
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
