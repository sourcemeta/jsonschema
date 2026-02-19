#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/schema.json
Installed      : $(realpath "$TMP")/project/vendor/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

HASH="$(shasum -a 256 < "$TMP/project/vendor/schema.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": {
      "path": "./vendor/schema.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"

"$1" validate "$TMP/project/vendor/schema.json" "$TMP/source/schema.json" 2>/dev/null
