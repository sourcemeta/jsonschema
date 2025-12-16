#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string",
      "default": 1
    }
  }
}
EOF

"$1" lint "$TMP/schema.json" --fix > "$TMP/result.txt" 2>&1

cat << 'EOF' > "$TMP/output.txt"
EOF

diff "$TMP/result.txt" "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Test",
  "description": "Test schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
