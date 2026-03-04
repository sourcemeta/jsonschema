#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

printf 'foo: bar\nbaz: 1\n' | "$1" validate "$TMP/schema.json" - >"$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
fail: <stdin>
error: Schema validation failure
  The value was expected to be of type string but it was of type object
    at instance location "" (line 1, column 1)
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
printf 'foo: bar\nbaz: 1\n' | "$1" validate "$TMP/schema.json" - --json >"$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.txt"
<stdin>
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/type",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/type",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 3, 0 ],
      "error": "The value was expected to be of type string but it was of type object"
    }
  ]
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
