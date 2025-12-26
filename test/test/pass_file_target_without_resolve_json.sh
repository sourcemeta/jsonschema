#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/this/is/a/very/very/very/long/path"

cat << 'EOF' > "$TMP/this/is/a/very/very/very/long/path/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/this/is/a/very/very/very/long/path/test.json"
{
  "target": "./schema.json",
  "tests": [
    {
      "description": "Valid string",
      "valid": true,
      "data": "foo"
    },
    {
      "description": "Invalid type",
      "valid": false,
      "data": 1
    }
  ]
}
EOF

"$1" test "$TMP/this/is/a/very/very/very/long/path/test.json" --json > "$TMP/output.json" 2>&1

# Validate against CTRF schema
CTRF_SCHEMA="$(dirname "$0")/../../vendor/ctrf/specification/schema-0.0.0.json"
"$1" validate "$CTRF_SCHEMA" "$TMP/output.json"

# Remove dynamic fields for comparison
sed -e '/"duration":/d' \
    -e '/"start":/d' \
    -e '/"stop":/d' \
    -e '/"threadId":/d' \
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
      "passed": 2,
      "failed": 0,
      "pending": 0,
      "skipped": 0,
      "other": 0,
    },
    "tests": [
      {
        "name": "Valid string",
        "status": "passed",
        "suite": [
          "file://$(realpath "$TMP")/this/is/a/very/very/very/long/path/schema.json"
        ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/this/is/a/very/very/very/long/path/test.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "Invalid type",
        "status": "passed",
        "suite": [
          "file://$(realpath "$TMP")/this/is/a/very/very/very/long/path/schema.json"
        ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/this/is/a/very/very/very/long/path/test.json",
        "line": 9,
        "retries": 0,
        "flaky": false,
      }
    ]
  }
}
EOF

diff "$TMP/output_filtered.json" "$TMP/expected.json"
