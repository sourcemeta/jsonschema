#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "id": "https://example.com/test",
  "type": "integer",
  "minimum": 0
}
EOF

"$1" upgrade -t 2020-12 "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "type": "integer",
  "minimum": 0
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
