#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

"$1" metaschema "$TMP/schema.json" --json > "$TMP/output.json" && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/properties/type/anyOf",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties/type/anyOf",
      "instanceLocation": "/type",
      "error": "The integer value was expected to validate against at least one of the 2 given subschemas"
    },
    {
      "keywordLocation": "/properties",
      "absoluteKeywordLocation": "http://json-schema.org/draft-04/schema#/properties",
      "instanceLocation": "",
      "error": "The object value was expected to validate against the 33 defined properties subschemas"
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
