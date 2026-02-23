#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "https://example.com/other": "./vendor/user.json"
  }
}
EOF

cd "$TMP/project"
"$1" install "https://example.com/schema" "./vendor/user.json" \
  > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: Multiple dependencies cannot point to the same path
  at file path $(realpath "$TMP")/project/jsonschema.json
  at location "/dependencies/https:~1~1example.com~1other"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json "https://example.com/schema" "./vendor/user.json" \
  > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Multiple dependencies cannot point to the same path",
  "filePath": "$(realpath "$TMP")/project/jsonschema.json",
  "location": "/dependencies/https:~1~1example.com~1other"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

test ! -f "$TMP/project/jsonschema.lock.json"
