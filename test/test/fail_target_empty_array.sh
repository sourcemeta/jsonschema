#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": [],
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
error: The test document \`target\` array must contain at least one URI
  at line 2
  at column 3
  at file path $(realpath "$TMP")/test.json
  at location "/target"

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
{
  "error": "The test document \`target\` array must contain at least one URI",
  "line": 2,
  "column": 3,
  "filePath": "$(realpath "$TMP")/test.json",
  "location": "/target"
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
