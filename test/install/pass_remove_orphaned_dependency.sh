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
"$1" install > "$TMP/first_output.txt" 2>&1

cat << EOF > "$TMP/expected_first.txt"
Fetching       : file://$(realpath "$TMP")/source/a.json
Installed      : $(realpath "$TMP")/project/vendor/a.json
Fetching       : file://$(realpath "$TMP")/source/b.json
Installed      : $(realpath "$TMP")/project/vendor/b.json
EOF

diff "$TMP/first_output.txt" "$TMP/expected_first.txt"

cat << EOF > "$TMP/expected_a.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/a",
  "type": "string"
}
EOF

diff "$TMP/project/vendor/a.json" "$TMP/expected_a.json"

cat << EOF > "$TMP/expected_b.json"
{
  "\$schema": "https://json-schema.org/draft/2020-12/schema",
  "\$id": "https://example.com/b",
  "type": "integer"
}
EOF

diff "$TMP/project/vendor/b.json" "$TMP/expected_b.json"

HASH_A="$(shasum -a 256 "$TMP/project/vendor/a.json" | cut -d ' ' -f 1)"
HASH_B="$(shasum -a 256 "$TMP/project/vendor/b.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": {
      "path": "./vendor/a.json",
      "hash": "$HASH_A",
      "hashAlgorithm": "sha256"
    },
    "file://$(realpath "$TMP")/source/b.json": {
      "path": "./vendor/b.json",
      "hash": "$HASH_B",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": "./vendor/a.json"
  }
}
EOF

"$1" install > "$TMP/second_output.txt" 2>&1

cat << EOF > "$TMP/expected_second.txt"
Up to date     : file://$(realpath "$TMP")/source/a.json
Orphaned       : file://$(realpath "$TMP")/source/b.json
EOF

diff "$TMP/second_output.txt" "$TMP/expected_second.txt"

test ! -f "$TMP/project/vendor/b.json"

diff "$TMP/project/vendor/a.json" "$TMP/expected_a.json"

cat << EOF > "$TMP/expected_lock_after.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": {
      "path": "./vendor/a.json",
      "hash": "$HASH_A",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock_after.json"
