#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/one.json"
{
  "id": "https://example.com/one",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/schemas/two.json"
{
  "id": "https://example.com/two",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "number"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": [
    "https://example.com/one",
    "https://example.com/two"
  ],
  "tests": [
    {
      "description": "String is valid",
      "valid": true,
      "data": "foo"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schemas" 1> "$TMP/output.txt" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/test.json:
  https://example.com/two:
    2/2 FAIL String is valid

error: Schema validation failure
  The value was expected to be of type number but it was of type string
    at instance location ""
    at evaluate path "/type"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"

"$1" test "$TMP/test.json" --resolve "$TMP/schemas" --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
test "$EXIT_CODE" = "2"

CTRF_SCHEMA="$(dirname "$0")/../../vendor/ctrf/specification/schema-0.0.0.json"
"$1" validate "$CTRF_SCHEMA" "$TMP/output.json"

sed -e '/"duration":/d' \
    -e '/"start":/d' \
    -e '/"stop":/d' \
    -e '/"threadId":/d' \
    -e '/"trace":/d' \
    "$TMP/output.json" > "$TMP/output_filtered.json"

VERSION=$("$1" --version)

cat << EOF > "$TMP/expected.json"
{
  "reportFormat": "CTRF",
  "specVersion": "0.0.0",
  "results": {
    "tool": {
      "name": "jsonschema",
      "version": "$VERSION"
    },
    "summary": {
      "tests": 2,
      "passed": 1,
      "failed": 1,
      "pending": 0,
      "skipped": 0,
      "other": 0,
    },
    "tests": [
      {
        "name": "String is valid",
        "status": "passed",
        "suite": [ "https://example.com/one" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 7,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "String is valid",
        "status": "failed",
        "suite": [ "https://example.com/two" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 7,
        "retries": 0,
        "flaky": false,
      }
    ]
  }
}
EOF

diff "$TMP/output_filtered.json" "$TMP/expected.json"
