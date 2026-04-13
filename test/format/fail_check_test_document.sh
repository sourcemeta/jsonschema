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
      "description": "I expect to pass",
      "valid": true,
      "data": {
        "bar": 1,
        "foo": 1
      }
    }
  ],
  "target": "https://example.com/my-schema"
}
EOF

"$1" fmt "$TMP/test.json" --check > "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
Interpreting as a test file: $(realpath "$TMP")/test.json
fail: $(realpath "$TMP")/test.json

Run the \`fmt\` command without \`--check/-c\` to fix the formatting
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
