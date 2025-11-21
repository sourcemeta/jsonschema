#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/this/is/a/very/very/very/long/path"

cat << 'EOF' > "$TMP/this/is/a/very/very/very/long/path/schema.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

"$1" fmt "$TMP/this/is/a/very/very/very/long/path/schema.json" \
  --check --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    "$(realpath "$TMP")/this/is/a/very/very/very/long/path/schema.json"
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
