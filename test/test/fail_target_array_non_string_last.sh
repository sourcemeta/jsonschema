#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": [
    "https://example.com/one",
    1
  ],
  "tests": [
    {
      "valid": true,
      "data": {}
    }
  ]
}
EOF

"$1" test "$TMP/test.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: Each entry in the test document \`target\` array must be a URI
  at line 4
  at column 5
  at file path $(realpath "$TMP")/test.json
  at location "/target/1"

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "Each entry in the test document \`target\` array must be a URI",
  "line": 4,
  "column": 5,
  "filePath": "$(realpath "$TMP")/test.json",
  "location": "/target/1"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
