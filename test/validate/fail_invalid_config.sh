#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": 1
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" >"$TMP/output.txt" 2>&1 \
  && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The defaultDialect property must be a string
  at file path $(realpath "$TMP")/jsonschema.json
  at location "/defaultDialect"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
