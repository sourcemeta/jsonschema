#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/user.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/user.json": "./vendor/user.json"
  }
}
EOF

cd "$TMP/project"
"$1" install --json > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
{
  "events": [
    {
      "type": "fetching",
      "uri": "file://$(realpath "$TMP")/source/user.json"
    },
    {
      "type": "installed",
      "uri": "file://$(realpath "$TMP")/source/user.json",
      "path": "$(realpath "$TMP")/project/vendor/user.json"
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_schema.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "\$id": "file://$(realpath "$TMP")/source/user.json"
}
EOF

diff "$TMP/project/vendor/user.json" "$TMP/expected_schema.json"

HASH="$(shasum -a 256 < "$TMP/project/vendor/user.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/user.json": {
      "path": "$(realpath "$TMP")/project/vendor/user.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
