#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << EOF > "$TMP/source/metaschema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "file://$(realpath "$TMP")/source/metaschema.json",
  "\$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true
  }
}
EOF

cat << EOF > "$TMP/source/schema_a.json"
{
  "\$schema": "file://$(realpath "$TMP")/source/metaschema.json",
  "type": "object"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/metaschema.json": "./vendor/meta.json",
    "file://$(realpath "$TMP")/source/schema_a.json": "./vendor/a.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

test -f "$TMP/project/vendor/meta.json"
test -f "$TMP/project/vendor/a.json"

HASH_META="$(cat "$TMP/project/vendor/meta.json" | shasum -a 256 | cut -d ' ' -f 1)"
HASH_A="$(cat "$TMP/project/vendor/a.json" | shasum -a 256 | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/metaschema.json": {
      "path": "$(realpath "$TMP")/project/vendor/meta.json",
      "hash": "${HASH_META}",
      "hashAlgorithm": "sha256"
    },
    "file://$(realpath "$TMP")/source/schema_a.json": {
      "path": "$(realpath "$TMP")/project/vendor/a.json",
      "hash": "${HASH_A}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
