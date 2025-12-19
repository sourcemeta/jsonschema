#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "A test schema",
  "examples": [ "foo" ],
  "type": "string"
}
EOF

"$1" lint "$TMP/schema.custom" 2>&1

cat << 'EOF' > "$TMP/expected.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "A test schema",
  "examples": [ "foo" ],
  "type": "string"
}
EOF

diff "$TMP/schema.custom" "$TMP/expected.json"
