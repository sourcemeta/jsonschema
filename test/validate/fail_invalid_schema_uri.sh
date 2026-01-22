#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "not a valid uri",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" >"$TMP/output.txt" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The dialect is not a valid URI
  at value not a valid uri
  at keyword \$schema
  at file path $(realpath "$TMP")/schema.json

Are you sure the input is a valid JSON Schema and it is valid according to its meta-schema?
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
