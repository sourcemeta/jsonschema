#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "x-jsonld-type": "https://schema.org/Person",
  "properties": {
    "name": { "type": "string", "x-jsonld-id": "https://schema.org/name" },
    "email": { "type": "string", "x-jsonld-id": "https://schema.org/email" }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "name": 1 }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location "/name" (line 1, column 3)
    at evaluate path "/properties/name/type"
  The object value was expected to validate against the defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/name/type",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties/name/type",
      "instanceLocation": "/name",
      "instancePosition": [ 1, 3, 1, 11 ],
      "error": "The value was expected to be of type string but it was of type integer"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 13 ],
      "error": "The object value was expected to validate against the defined properties subschemas"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
