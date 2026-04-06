#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "examples": [ { "name": "John" } ],
  "$ref": "https://example.com/my-defs"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/my-defs": "./does-not-exist.json"
  }
}
EOF

BIN="$(realpath "$1")"
cd "$TMP"
"$BIN" lint schema.json > "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
error: The resolve target does not exist on the filesystem
  at resolve path $(realpath "$TMP")/does-not-exist.json
  at line 3
  at column 5
  at file path $(realpath "$TMP")/jsonschema.json
  at location "/resolve/https:~1~1example.com~1my-defs"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$BIN" lint schema.json --json > "$TMP/output_json.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected_json.txt"
{
  "error": "The resolve target does not exist on the filesystem",
  "resolvePath": "$(realpath "$TMP")/does-not-exist.json",
  "line": 3,
  "column": 5,
  "filePath": "$(realpath "$TMP")/jsonschema.json",
  "location": "/resolve/https:~1~1example.com~1my-defs"
}
EOF

diff "$TMP/output_json.txt" "$TMP/expected_json.txt"
