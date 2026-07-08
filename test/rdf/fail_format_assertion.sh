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
  "properties": {
    "email": {
      "type": "string",
      "format": "email",
      "x-jsonld-id": "https://schema.org/email"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "email": "not an email" }
EOF

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
[
  {
    "https://schema.org/email": [
      {
        "@value": "not an email"
      }
    ]
  }
]
EOF

diff "$TMP/output.json" "$TMP/expected.json"

"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --format-assertion \
  2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The string value "not an email" was expected to represent a valid email address
    at instance location "/email" (line 1, column 3)
    at evaluate path "/properties/email/format"
  The object value was expected to validate against the single defined property subschema
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
"$1" rdf "$TMP/schema.json" "$TMP/instance.json" --format-assertion --json \
  > "$TMP/stdout.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/email/format",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties/email/format",
      "instanceLocation": "/email",
      "instancePosition": [ 1, 3, 1, 25 ],
      "error": "The string value \"not an email\" was expected to represent a valid email address"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 27 ],
      "error": "The object value was expected to validate against the single defined property subschema"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
