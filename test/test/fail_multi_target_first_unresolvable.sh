#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com/two",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": [
    "https://example.com/missing",
    "https://example.com/two"
  ],
  "tests": [
    {
      "valid": true,
      "data": "foo"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --verbose 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: Could not resolve the reference to an external schema
  at identifier https://example.com/missing
  at file path $(realpath "$TMP")/test.json

This is likely because you forgot to import such schema using \`--resolve/-r\`
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.json"
{
  "error": "Could not resolve the reference to an external schema",
  "identifier": "https://example.com/missing",
  "filePath": "$(realpath "$TMP")/test.json"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
