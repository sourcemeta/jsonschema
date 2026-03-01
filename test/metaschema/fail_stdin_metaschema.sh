#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" metaschema - 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF
# Metaschema validation failure
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
fail: <stdin>
error: Schema validation failure
  The integer value 1 was expected to equal one of the following values: "array", "boolean", "integer", "null", "number", "object", and "string"
    at instance location "/type" (line 3, column 3)
    at evaluate path "/properties/type/anyOf/0/$ref/enum"
  The integer value was expected to validate against the referenced schema
    at instance location "/type" (line 3, column 3)
    at evaluate path "/properties/type/anyOf/0/$ref"
  The value was expected to be of type array but it was of type integer
    at instance location "/type" (line 3, column 3)
    at evaluate path "/properties/type/anyOf/1/type"
  The integer value was expected to validate against at least one of the 2 given subschemas
    at instance location "/type" (line 3, column 3)
    at evaluate path "/properties/type/anyOf"
  The object value was expected to validate against the 33 defined properties subschemas
    at instance location "" (line 1, column 1)
    at evaluate path "/properties"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

# JSON error
cat << 'EOF' | "$1" metaschema - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
<stdin>
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/type/anyOf/0/$ref/enum",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/definitions/simpleTypes/enum",
      "instanceLocation": "/type",
      "instancePosition": [ 3, 3, 3, 11 ],
      "error": "The integer value 1 was expected to equal one of the following values: \"array\", \"boolean\", \"integer\", \"null\", \"number\", \"object\", and \"string\""
    },
    {
      "keywordLocation": "/properties/type/anyOf/0/$ref",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf/0/$ref",
      "instanceLocation": "/type",
      "instancePosition": [ 3, 3, 3, 11 ],
      "error": "The integer value was expected to validate against the referenced schema"
    },
    {
      "keywordLocation": "/properties/type/anyOf/1/type",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf/1/type",
      "instanceLocation": "/type",
      "instancePosition": [ 3, 3, 3, 11 ],
      "error": "The value was expected to be of type array but it was of type integer"
    },
    {
      "keywordLocation": "/properties/type/anyOf",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf",
      "instanceLocation": "/type",
      "instancePosition": [ 3, 3, 3, 11 ],
      "error": "The integer value was expected to validate against at least one of the 2 given subschemas"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 4, 1 ],
      "error": "The object value was expected to validate against the 33 defined properties subschemas"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
