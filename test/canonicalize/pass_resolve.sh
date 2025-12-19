#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/referenced"
}
EOF

cat << 'EOF' > "$TMP/referenced.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/referenced",
  "type": "string"
}
EOF

"$1" canonicalize "$TMP/schema.json" --resolve "$TMP/referenced.json" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/referenced"
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"
