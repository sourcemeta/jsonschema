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

cat << 'EOF' > "$TMP/project/jsonschema.lock.json"
{
  "foo": "bar"
}
EOF

cd "$TMP/project"
"$1" install --frozen > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Lock file is corrupted
  at file path $(realpath "$TMP")/project/jsonschema.lock.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --frozen --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Lock file is corrupted",
  "filePath": "$(realpath "$TMP")/project/jsonschema.lock.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
