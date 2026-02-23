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
  "type": 1
}
EOF

"$1" metaschema "$TMP/schema.json" --json > "$TMP/output.json" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Metaschema validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/type/anyOf/0/\$ref/enum",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/definitions/simpleTypes/enum",
      "instanceLocation": "/type",
      "instancePosition": [ 5, 3, 5, 11 ],
      "error": "The integer value 1 was expected to equal one of the following values: \"array\", \"boolean\", \"integer\", \"null\", \"number\", \"object\", and \"string\""
    },
    {
      "keywordLocation": "/properties/type/anyOf/0/\$ref",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf/0/\$ref",
      "instanceLocation": "/type",
      "instancePosition": [ 5, 3, 5, 11 ],
      "error": "The integer value was expected to validate against the referenced schema"
    },
    {
      "keywordLocation": "/properties/type/anyOf/1/type",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf/1/type",
      "instanceLocation": "/type",
      "instancePosition": [ 5, 3, 5, 11 ],
      "error": "The value was expected to be of type array but it was of type integer"
    },
    {
      "keywordLocation": "/properties/type/anyOf",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf",
      "instanceLocation": "/type",
      "instancePosition": [ 5, 3, 5, 11 ],
      "error": "The integer value was expected to validate against at least one of the 2 given subschemas"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 6, 1 ],
      "error": "The object value was expected to validate against the 33 defined properties subschemas"
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
