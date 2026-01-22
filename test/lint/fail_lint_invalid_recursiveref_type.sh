#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$recursiveRef": 123
}
EOF

"$1" lint "$TMP/schema.json" >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Invalid recursive reference value
  at value 123
  at keyword \$recursiveRef
  at file path $(realpath "$TMP")/schema.json

Are you sure the input is a valid JSON Schema and it is valid according to its meta-schema?
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "error": "Invalid recursive reference value",
  "value": "123",
  "keyword": "\$recursiveRef",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
