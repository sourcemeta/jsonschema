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
  "$defs": {
    "PositiveInt": {
      "type": "integer",
      "minimum": 1
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
-5
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint "/\$defs/PositiveInt" --verbose > "$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The integer value -5 was expected to be greater than or equal to the integer 1
    at instance location "" (line 1, column 1)
    at evaluate path "/minimum"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --entrypoint "/\$defs/PositiveInt" --json > "$TMP/stdout.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/minimum",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/\$defs/PositiveInt/minimum",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 2 ],
      "error": "The integer value -5 was expected to be greater than or equal to the integer 1"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
