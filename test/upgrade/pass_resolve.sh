#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/string.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/string",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/string"
}
EOF

"$1" upgrade --resolve "$TMP/string.json" "$TMP/schema.json" > "$TMP/output.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/string"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
