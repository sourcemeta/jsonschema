#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "tests": [
    {
      "valid": true,
      "data": {}
    },
    {
      "valid": true,
      "data": { "type": 1 }
    }
  ]
}
EOF

"$1" test "$TMP/test.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: The test document must contain a \`target\` property
  at line 1
  at column 1
  at file path $(realpath "$TMP")/test.json
  at location ""

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
"$1" test "$TMP/test.json" --json > "$TMP/stdout.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
{
  "error": "The test document must contain a \`target\` property",
  "line": 1,
  "column": 1,
  "filePath": "$(realpath "$TMP")/test.json",
  "location": ""
}
EOF

diff "$TMP/stdout.txt" "$TMP/expected.txt"
