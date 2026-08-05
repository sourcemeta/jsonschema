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
      "description": "Second test",
      "valid": true,
      "data": "bar"
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/3.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Third test",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

cat << 'EOF' > "$TMP/tests/4.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Fourth test",
      "valid": true,
      "data": "baz"
    }
  ]
}
EOF

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
      "tests": 4,
      "passed": 4,
      "failed": 0,
      "pending": 0,
      "skipped": 0,
      "other": 0,
    },
    "tests": [
      {
        "name": "First test",
        "status": "passed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/tests/1.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "Second test",
        "status": "passed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/tests/2.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "Third test",
        "status": "passed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/tests/3.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "Fourth test",
        "status": "passed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/tests/4.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
      }
    ]
  }
}
EOF

CTRF_SCHEMA="$(dirname "$0")/../../vendor/ctrf/specification/schema-0.0.0.json"

for _ in 1 2 3 4 5
do
  "$1" test "$TMP/tests" --resolve "$TMP/schema.json" --json --jobs 2 \
    > "$TMP/output.json" 2>&1

  "$1" validate "$CTRF_SCHEMA" "$TMP/output.json"

  sed -e '/"duration":/d' \
      -e '/"start":/d' \
      -e '/"stop":/d' \
      -e '/"threadId":/d' \
      "$TMP/output.json" > "$TMP/output_filtered.json"

  diff "$TMP/output_filtered.json" "$TMP/expected.json"
done

"$1" test "$TMP/tests" --resolve "$TMP/schema.json" --json --jobs 1 \
  > "$TMP/output.json" 2>&1

"$1" validate "$CTRF_SCHEMA" "$TMP/output.json"

sed -e '/"duration":/d' \
    -e '/"start":/d' \
    -e '/"stop":/d' \
    -e '/"threadId":/d' \
    "$TMP/output.json" > "$TMP/output_filtered.json"

diff "$TMP/output_filtered.json" "$TMP/expected.json"
