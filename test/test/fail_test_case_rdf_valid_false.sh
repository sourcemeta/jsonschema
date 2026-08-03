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
      "valid": false,
      "data": {},
      "rdf": []
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
error: Test case documents may only set the \`rdf\` or \`rdfPath\` property when the \`valid\` property is set to true
  at line 4
  at column 5
  at file path $(realpath "$TMP")/test.json
  at location "/tests/0"

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

# JSON error
"$1" test "$TMP/test.json" --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.json"
{
  "error": "Test case documents may only set the \`rdf\` or \`rdfPath\` property when the \`valid\` property is set to true",
  "line": 4,
  "column": 5,
  "filePath": "$(realpath "$TMP")/test.json",
  "location": "/tests/0"
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
