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
    "file://$(realpath "$TMP")/source/schema_a.json": "./vendor/a.json",
    "file://$(realpath "$TMP")/source/metaschema.json": "./vendor/meta.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/metaschema.json
Installed      : $(realpath "$TMP")/project/vendor/meta.json
Fetching       : file://$(realpath "$TMP")/source/schema_a.json
Installed      : $(realpath "$TMP")/project/vendor/a.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_meta.json"
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

diff "$TMP/project/vendor/meta.json" "$TMP/expected_meta.json"

cat << EOF > "$TMP/expected_a.json"
{
  "\$schema": "file://$(realpath "$TMP")/source/metaschema.json",
  "type": "object",
  "\$id": "file://$(realpath "$TMP")/source/schema_a.json",
  "\$defs": {
    "file://$(realpath "$TMP")/source/metaschema.json": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "file://$(realpath "$TMP")/source/metaschema.json",
      "\$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/applicator": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      }
    }
  }
}
EOF

diff "$TMP/project/vendor/a.json" "$TMP/expected_a.json"

HASH_A="$(shasum -a 256 < "$TMP/project/vendor/a.json" | cut -d ' ' -f 1)"
HASH_META="$(shasum -a 256 < "$TMP/project/vendor/meta.json" | cut -d ' ' -f 1)"

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
