#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
{"$id":"https://example.com","$schema":"https://json-schema.org/draft/2020-12/schema","type":"string"}
EOF

"$1" fmt "$TMP/schema.custom"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "type": "string"
}
EOF

diff "$TMP/schema.custom" "$TMP/expected.json"
