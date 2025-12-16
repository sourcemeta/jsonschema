#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://spec.openapis.org/oas/3.2/dialect/2025-09-17",
  "title": "Test",
  "description": "Test schema",
  "type": "string",
  "externalDocs": {
    "url": "https://example.com"
  }
}
EOF

"$1" lint "$TMP/schema.json" > "$TMP/result.txt" 2>&1

cat << 'EOF' > "$TMP/output.txt"
EOF

diff "$TMP/result.txt" "$TMP/output.txt"
