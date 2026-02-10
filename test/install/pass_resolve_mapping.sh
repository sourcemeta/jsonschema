#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/main.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/main",
  "$ref": "https://example.com/types/name"
}
EOF

cat << 'EOF' > "$TMP/project/name.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/types/name",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "resolve": {
    "https://example.com/types/name": "./name.json"
  },
  "dependencies": {
    "file://$(realpath "$TMP")/source/main.json": "./vendor/main.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

test -f "$TMP/project/vendor/main.json"
test -f "$TMP/project/jsonschema.lock.json"
