#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "not a valid uri"
}
EOF

"$1" lint "$TMP/schema.json" >"$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The identifier is not a valid URI
  at value not a valid uri
  at keyword \$id
  at file path $(realpath "$TMP")/schema.json

Are you sure the input is a valid JSON Schema and it is valid according to its meta-schema?
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" lint "$TMP/schema.json" --json >"$TMP/output.json" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Schema input error
test "$EXIT_CODE" = "4" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "error": "The identifier is not a valid URI",
  "value": "not a valid uri",
  "keyword": "\$id",
  "filePath": "$(realpath "$TMP")/schema.json"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
