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
  "$ref": "https://example.com/referenced"
}
EOF

cat << 'EOF' > "$TMP/project/local_override.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/referenced",
  "type": "string",
  "description": "local override"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "resolve": {
    "https://example.com/referenced": "./local_override.json"
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
  "\$ref": "https://example.com/referenced",
  "\$defs": {
    "https://example.com/referenced": {
      "\$schema": "https://json-schema.org/draft/2020-12/schema",
      "\$id": "https://example.com/referenced",
      "type": "string",
      "description": "local override"
    }
  }
}
EOF

diff "$TMP/project/vendor/main.json" "$TMP/expected_schema.json"

HASH="$(shasum -a 256 < "$TMP/project/vendor/main.json" | cut -d ' ' -f 1)"

cat << EOF > "$TMP/expected_lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/main.json": {
      "path": "./vendor/main.json",
      "hash": "${HASH}",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

diff "$TMP/project/jsonschema.lock.json" "$TMP/expected_lock.json"
