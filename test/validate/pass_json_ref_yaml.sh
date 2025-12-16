#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$ref": "http://example.com/schema.yaml"
}
EOF

cat << 'EOF' > "$TMP/schema.yaml"
$id: http://example.com/schema.yaml
$schema: https://json-schema.org/draft/2020-12/schema
type: string
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

"$1" validate "$TMP/schema.json" --resolve "$TMP/schema.yaml" "$TMP/instance.json"
