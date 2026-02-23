#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://json-schema.org/draft/2020-12/schema",
  "tests": {
    "valid": true,
    "data": {}
  }
}
EOF

"$1" test "$TMP/test.json" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6" || exit 1

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
error: The test document \`tests\` property must be an array
  at line 3
  at column 3
  at file path $(realpath "$TMP")/test.json
  at location "/tests"

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
