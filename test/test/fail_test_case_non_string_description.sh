#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": "http://json-schema.org/draft-04/schema#",
  "tests": [
    {
      "description": "foo",
      "valid": true,
      "data": {}
    },
    {
      "description": 1,
      "valid": true,
      "data": {}
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
error: If you set a test case description, it must be a string
  at line 10
  at column 7
  at file path $(realpath "$TMP")/test.json
  at location "/tests/1/description"

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
