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

cd "$TMP/project"
"$1" install --frozen --force > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected.txt"
error: The --frozen and --force options cannot be used together
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --frozen --force --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "1" || exit 1

cat << 'EOF' > "$TMP/expected_json.txt"
{
  "error": "The --frozen and --force options cannot be used together"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
