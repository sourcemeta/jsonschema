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
      "data": "foo"
    },
    {
      "description": "First failure",
      "valid": true,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/3.json"
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

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/tests/1.json: PASS 1/1
$(realpath "$TMP")/tests/2.json:
  2/2 FAIL First failure

error: Schema validation failure
  The value was expected to be of type string but it was of type integer
    at instance location ""
    at evaluate path "/type"
$(realpath "$TMP")/tests/3.json: PASS 1/1
$(realpath "$TMP")/tests/4.json: PASS 1/1
EOF

sort "$TMP/expected.txt" > "$TMP/expected_sorted.txt"

for _ in 1 2 3 4 5
do
  "$1" test "$TMP/tests" --resolve "$TMP/schema.json" --jobs 3 \
    1> "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
  # Test assertion failure
  test "$EXIT_CODE" = "2"
  sort "$TMP/output.txt" > "$TMP/output_sorted.txt"
  diff "$TMP/output_sorted.txt" "$TMP/expected_sorted.txt"
done

"$1" test "$TMP/tests" --resolve "$TMP/schema.json" --jobs 1 \
  1> "$TMP/output.txt" 2>&1 && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

diff "$TMP/output.txt" "$TMP/expected.txt"
