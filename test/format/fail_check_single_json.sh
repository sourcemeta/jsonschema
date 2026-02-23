#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/this/is/a/very/very/very/very/very/very/long/path"

cat << 'EOF' > "$TMP/this/is/a/very/very/very/very/very/very/long/path/schema.json"
{
  "type": "string",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema"
}
EOF

"$1" fmt "$TMP/this/is/a/very/very/very/very/very/very/long/path/schema.json" \
  --check --json >"$TMP/output.json" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Format check failure
test "$EXIT_CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    "$(realpath "$TMP")/this/is/a/very/very/very/very/very/very/long/path/schema.json"
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
