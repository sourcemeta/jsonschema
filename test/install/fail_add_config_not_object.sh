#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/project"

cat << 'EOF' > "$TMP/project/jsonschema.json"
[]
EOF

cd "$TMP/project"
"$1" install "https://example.com/foo" "./vendor/foo.json" \
  > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
error: The configuration must be an object
  at file path $(realpath "$TMP")/project/jsonschema.json
  at location ""
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json "https://example.com/foo" "./vendor/foo.json" \
  > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The configuration must be an object",
  "filePath": "$(realpath "$TMP")/project/jsonschema.json",
  "location": ""
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

test ! -f "$TMP/project/jsonschema.lock.json"
