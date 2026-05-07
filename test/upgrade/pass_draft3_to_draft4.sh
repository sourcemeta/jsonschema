#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-03/schema#",
  "id": "https://example.com/test",
  "type": "integer",
  "divisibleBy": 2
}
EOF

"$1" upgrade --to draft4 "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "id": "https://example.com/test",
  "type": "integer",
  "multipleOf": 2
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
