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

echo '123' | "$1" validate "$TMP/schema.json" - --json >"$TMP/stdout.json" 2>"$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected_stderr.txt"
$(pwd)
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    {
      "keywordLocation": "/type",
      "absoluteKeywordLocation": "file://$(realpath "$TMP")/schema.json#/type",
      "instanceLocation": "",
      "instancePosition": [ 1, 1, 1, 3 ],
      "error": "The value was expected to be of type string but it was of type integer"
    }
  ]
}
EOF

diff "$TMP/stdout.json" "$TMP/expected.json"
