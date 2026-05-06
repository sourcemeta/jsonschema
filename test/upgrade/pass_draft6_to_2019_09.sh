#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "$id": "https://example.com/test",
  "type": "integer",
  "minimum": 0
}
EOF

"$1" upgrade --to 2019-09 "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2019-09/schema",
  "$id": "https://example.com/test",
  "type": "integer",
  "minimum": 0
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
