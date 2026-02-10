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
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/main.json
Installed      : $(realpath "$TMP")/project/vendor/main.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/main",
  "\$ref": "https://example.com/types/name",
  "\$defs": {
    "https://example.com/types/name": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "https://example.com/types/name",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/project/vendor/main.json" "$TMP/expected_schema.json"

HASH="$(cat "$TMP/project/vendor/main.json" | shasum -a 256 | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/main.json": {
      "path": "$(realpath "$TMP")/project/vendor/main.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
