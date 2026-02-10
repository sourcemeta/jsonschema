#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/alpha.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/source/beta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "integer"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/alpha.json": "./vendor/alpha.json",
    "file://$(realpath "$TMP")/source/beta.json": "./vendor/beta.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Fetching       : file://$(realpath "$TMP")/source/alpha.json
Installed      : $(realpath "$TMP")/project/vendor/alpha.json
Fetching       : file://$(realpath "$TMP")/source/beta.json
Installed      : $(realpath "$TMP")/project/vendor/beta.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_alpha.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "\$id": "file://$(realpath "$TMP")/source/alpha.json"
}
EOF

diff "$TMP/project/vendor/alpha.json" "$TMP/expected_alpha.json"

cat << EOF > "$TMP/expected_beta.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "integer",
  "\$id": "file://$(realpath "$TMP")/source/beta.json"
}
EOF

diff "$TMP/project/vendor/beta.json" "$TMP/expected_beta.json"

HASH_ALPHA="$(cat "$TMP/project/vendor/alpha.json" | shasum -a 256 | cut -d ' ' -f 1)"
HASH_BETA="$(cat "$TMP/project/vendor/beta.json" | shasum -a 256 | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/alpha.json": {
      "path": "$(realpath "$TMP")/project/vendor/alpha.json",
      "hash": "${HASH_ALPHA}",
      "hashAlgorithm": "sha256"
    },
    "file://$(realpath "$TMP")/source/beta.json": {
      "path": "$(realpath "$TMP")/project/vendor/beta.json",
      "hash": "${HASH_BETA}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
