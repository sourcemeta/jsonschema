#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << 'EOF' > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file:///tmp/fake/schema.json": "./vendor/schema.json"
  }
}
EOF

echo "NOT VALID JSON" > "$TMP/project/jsonschema.lock.json"

cd "$TMP/project"
"$1" install --frozen > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Failed to parse the JSON document
  at line 1
  at column 1
  at file path $(realpath "$TMP")/project/jsonschema.lock.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --frozen --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Failed to parse the JSON document",
  "line": 1,
  "column": 1,
  "filePath": "$(realpath "$TMP")/project/jsonschema.lock.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
