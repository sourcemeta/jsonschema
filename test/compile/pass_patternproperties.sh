#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "Test",
  "description": "Test schema",
  "patternProperties": {
    "[\\-]": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{}
EOF

"$1" compile "$TMP/schema.json" > "$TMP/template.json"
"$1" validate --template "$TMP/template.json" "$TMP/schema.json" "$TMP/instance.json"
