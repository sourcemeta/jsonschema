#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/source" "$TMP/project"

cat << 'EOF' > "$TMP/source/a.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/source/b.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "integer"
}
EOF

cat << EOF > "$TMP/project/jsonschema.json"
{
  "dependencies": {
    "file://$(realpath "$TMP")/source/a.json": "./vendor/schema.json",
    "file://$(realpath "$TMP")/source/b.json": "./vendor/schema.json"
  }
}
EOF

ESCAPED_TMP="$(realpath "$TMP" | sed 's|/|~1|g')"

cd "$TMP/project"
"$1" install > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
error: Multiple dependencies cannot point to the same path
  at file path $(realpath "$TMP")/project/jsonschema.json
  at location "/dependencies/file:~1~1${ESCAPED_TMP}~1source~1a.json"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" install --json > "$TMP/output_json.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "Multiple dependencies cannot point to the same path",
  "filePath": "$(realpath "$TMP")/project/jsonschema.json",
  "location": "/dependencies/file:~1~1${ESCAPED_TMP}~1source~1a.json"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"

test ! -f "$TMP/project/jsonschema.lock.json"
