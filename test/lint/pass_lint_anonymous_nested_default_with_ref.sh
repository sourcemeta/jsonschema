#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "items": {
    "default": "foo",
    "$ref": "./other.json"
  }
}
EOF

cat << 'EOF' > "$TMP/other.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "string"
}
EOF

"$1" lint "$TMP/schema.json" --resolve "$TMP/other.json" --verbose
