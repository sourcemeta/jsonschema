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
  "format": "email"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"not-an-email"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --format-assertion \
  2> "$TMP/stderr.txt" && EXIT_CODE="$?" || EXIT_CODE="$?"
# Validation failure
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
fail: $(realpath "$TMP")/instance.json
error: Schema validation failure
  The string value "not-an-email" was expected to represent a valid email address
    at instance location "" (line 1, column 1)
    at evaluate path "/format"
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
