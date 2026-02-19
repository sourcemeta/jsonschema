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
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/custom_meta.json
Installed      : $(realpath "$TMP")/project/vendor/custom_meta.json
Fetching       : file://$(realpath "$TMP")/source/my_schema.json
Installed      : $(realpath "$TMP")/project/vendor/my_schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_meta.json"
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

diff "$TMP/project/vendor/custom_meta.json" "$TMP/expected_meta.json"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "file://$(realpath "$TMP")/source/custom_meta.json",
  "type": "object",
  "properties": {
    "name": {
      "type": "string"
    }
  },
  "\$id": "file://$(realpath "$TMP")/source/my_schema.json",
  "\$defs": {
    "file://$(realpath "$TMP")/source/custom_meta.json": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "file://$(realpath "$TMP")/source/custom_meta.json",
      "\$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/applicator": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      }
    }
  }
}
EOF

diff "$TMP/project/vendor/my_schema.json" "$TMP/expected_schema.json"

HASH_META="$(shasum -a 256 < "$TMP/project/vendor/custom_meta.json" | cut -d ' ' -f 1)"
HASH_SCHEMA="$(shasum -a 256 < "$TMP/project/vendor/my_schema.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/custom_meta.json": {
      "path": "./vendor/custom_meta.json",
      "hash": "${HASH_META}",
      "hashAlgorithm": "sha256"
    },
    "file://$(realpath "$TMP")/source/my_schema.json": {
      "path": "./vendor/my_schema.json",
      "hash": "${HASH_SCHEMA}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
