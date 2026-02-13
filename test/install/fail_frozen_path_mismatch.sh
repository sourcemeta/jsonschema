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
  "type": "string"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": "./vendor/schema.json"
  }
}
EOF

cat << EOF > "$TMP/project/jsonschema.lock.json"
{
  "version": 1,
  "dependencies": {
    "file://$(realpath "$TMP")/source/schema.json": {
      "path": "$(realpath "$TMP")/project/vendor/WRONG.json",
      "hash": "abc123",
      "hashAlgorithm": "sha256"
    }
  }
}
EOF

cp "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

cd "$TMP/project"
"$1" install --frozen > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
Path mismatch  : file://$(realpath "$TMP")/source/schema.json
error: Configured path does not match lock file in frozen mode
  at uri file://$(realpath "$TMP")/source/schema.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"

"$1" install --frozen --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "events": [
    {
      "type": "path-mismatch",
      "uri": "file://$(realpath "$TMP")/source/schema.json"
    },
    {
      "type": "error",
      "uri": "file://$(realpath "$TMP")/source/schema.json",
      "message": "Configured path does not match lock file in frozen mode"
    }
  ]
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

diff "$TMP/project/jsonschema.lock.json" "$TMP/lock_before.json"
