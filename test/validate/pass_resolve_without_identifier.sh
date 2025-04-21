#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

cat << 'EOF' > "$TMP/no-id.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "boolean"
}
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/no-id.json" 2>"$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
warning: No schema resources were imported from this file
  at $(realpath "$TMP")/no-id.json
Are you sure this schema sets any identifiers?
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
