#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << EOF > "$TMP/source/custom_meta.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "file://$(realpath "$TMP")/source/custom_meta.json",
  "\$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true
  }
}
EOF

cat << EOF > "$TMP/source/my_schema.json"
{
  "\$schema": "file://$(realpath "$TMP")/source/custom_meta.json",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    }
  }
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/custom_meta.json": "./vendor/custom_meta.json",
    "file://$(realpath "$TMP")/source/my_schema.json": "./vendor/my_schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

test -f "$TMP/project/vendor/custom_meta.json"
test -f "$TMP/project/vendor/my_schema.json"
test -f "$TMP/project/jsonschema.lock.json"
