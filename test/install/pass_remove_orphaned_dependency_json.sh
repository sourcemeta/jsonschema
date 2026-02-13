#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project/vendor"

cat << 'EOF' > "$TMP/source/a.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/a",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/source/b.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/b",
  "type": "integer"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": "./vendor/a.json",
    "file://$(realpath "$TMP")/source/b.json": "./vendor/b.json"
  }
}
EOF

cd "$TMP/project"
"$1" install > /dev/null 2>&1

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": "./vendor/a.json"
  }
}
EOF

"$1" install --json > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
{
  "events": [
    {
      "type": "up-to-date",
      "uri": "file://$(realpath "$TMP")/source/a.json"
    },
    {
      "type": "orphaned",
      "uri": "file://$(realpath "$TMP")/source/b.json"
    }
  ]
}
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

test ! -f "$TMP/project/vendor/b.json"

cat << EOF > "$TMP/expected_a.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/a",
  "type": "string"
}
EOF

diff "$TMP/project/vendor/a.json" "$TMP/expected_a.json"

HASH_A="$(shasum -a 256 < "$TMP/project/vendor/a.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": {
      "path": "$(realpath "$TMP")/project/vendor/a.json",
      "hash": "${HASH_A}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
