#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"
mkdir "$TMP/alpha"
mkdir "$TMP/alpha/tests"
mkdir "$TMP/beta"
mkdir "$TMP/beta/tests"

cat << 'EOF' > "$TMP/schemas/alpha.json"
{
  "id": "https://example.com/alpha",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/beta.json"
{
  "$id": "https://example.com/beta",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "integer"
}
EOF

cat << 'EOF' > "$TMP/alpha/jsonschema.json"
{
  "defaultDialect": "http://json-schema.org/draft-04/schema#",
  "resolve": {
    "https://example.com": "https://example.com/alpha"
  }
}
EOF

cat << 'EOF' > "$TMP/alpha/tests/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Second test",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/beta/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema",
  "resolve": {
    "https://example.com": "https://example.com/beta"
  }
}
EOF

cat << 'EOF' > "$TMP/beta/tests/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "First test",
      "valid": true,
      "data": 1
    },
    {
      "description": "Second test",
      "valid": false,
      "data": "foo"
    }
  ]
}
EOF

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/alpha/tests/test.json: PASS 2/2
$(realpath "$TMP")/beta/tests/test.json: PASS 2/2
EOF

sort "$TMP/expected.txt" > "$TMP/expected_sorted.txt"

for _ in 1 2 3 4 5
do
  "$1" test "$TMP/alpha/tests" "$TMP/beta/tests" \
    --resolve "$TMP/schemas" --jobs 2 \
    1> "$TMP/output.txt" 2>&1
  sort "$TMP/output.txt" > "$TMP/output_sorted.txt"
  diff "$TMP/output_sorted.txt" "$TMP/expected_sorted.txt"
done

"$1" test "$TMP/alpha/tests" "$TMP/beta/tests" \
  --resolve "$TMP/schemas" --jobs 1 \
  1> "$TMP/output.txt" 2>&1

diff "$TMP/output.txt" "$TMP/expected.txt"
