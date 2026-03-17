#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "pattern": "[invalid("
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" 2> "$TMP/stderr.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
error: Invalid regular expression
  at regex [invalid(
  at file path $(realpath "$TMP")/schema.json
  at location "/pattern"
  at base uri file://$(realpath "$TMP")/schema.json

Detailed regex error messages are not yet supported
Try tools like https://regex101.com to debug further
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --json > "$TMP/stdout.txt" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "4"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Invalid regular expression",
  "regex": "[invalid(",
  "filePath": "$(realpath "$TMP")/schema.json",
  "location": "/pattern",
  "baseURI": "file://$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
