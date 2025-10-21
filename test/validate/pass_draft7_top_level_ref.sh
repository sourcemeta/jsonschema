#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

# We expect `/type` to be ignored here
cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$ref": "#/definitions/foo",
  "type": "object",
  "definitions": {
    "foo": {}
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"foo"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json"
