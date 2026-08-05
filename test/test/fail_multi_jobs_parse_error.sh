#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

mkdir "$TMP/tests"

cat << 'EOF' > "$TMP/tests/1.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/2.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "bar"
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/3.json"
{
  "target": "https://example.com"
}
EOF

cat << 'EOF' > "$TMP/tests/4.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "baz"
    }
  ]
}
EOF

"$1" test "$TMP/tests" --resolve "$TMP/schema.json" --jobs 1 \
  1> "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Other input error
test "$EXIT_CODE" = "6"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/tests/1.json: PASS 1/1
$(realpath "$TMP")/tests/2.json: PASS 1/1
$(realpath "$TMP")/tests/3.json:
error: The test document must contain a \`tests\` property
  at line 1
  at column 1
  at file path $(realpath "$TMP")/tests/3.json
  at location ""

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

cat << EOF > "$TMP/expected_stderr.txt"
error: The test document must contain a \`tests\` property
  at line 1
  at column 1
  at file path $(realpath "$TMP")/tests/3.json
  at location ""

Learn more here: https://github.com/sourcemeta/jsonschema/blob/main/docs/test.markdown
EOF

cat << EOF > "$TMP/expected_stdout_tail.txt"
$(realpath "$TMP")/tests/3.json:
EOF

for _ in 1 2 3 4 5
do
  "$1" test "$TMP/tests" --resolve "$TMP/schema.json" --jobs 4 \
    1> "$TMP/stdout.txt" 2> "$TMP/stderr.txt" \
    && EXIT_CODE="$?" || EXIT_CODE="$?"
  # Other input error
  test "$EXIT_CODE" = "6"
  diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"
  tail -n 1 "$TMP/stdout.txt" > "$TMP/stdout_tail.txt"
  diff "$TMP/stdout_tail.txt" "$TMP/expected_stdout_tail.txt"
done
