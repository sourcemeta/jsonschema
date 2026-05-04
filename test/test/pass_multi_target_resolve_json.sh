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
  "type": [ "string", "number" ]
}
EOF

cat << 'EOF' > "$TMP/schemas/two.json"
{
  "id": "https://example.com/two",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": [ "string", "number" ]
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
    },
    {
      "description": "Object is invalid",
      "valid": false,
      "data": {}
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schemas" --json > "$TMP/output.json" 2>&1

CTRF_SCHEMA="$(dirname "$0")/../../vendor/ctrf/specification/schema-0.0.0.json"
"$1" validate "$CTRF_SCHEMA" "$TMP/output.json"

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
      "tests": 4,
      "passed": 4,
      "failed": 0,
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
        "name": "Object is invalid",
        "status": "passed",
        "suite": [ "https://example.com/one" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 12,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "String is valid",
        "status": "passed",
        "suite": [ "https://example.com/two" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 7,
        "retries": 0,
        "flaky": false,
      },
      {
        "name": "Object is invalid",
        "status": "passed",
        "suite": [ "https://example.com/two" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 12,
        "retries": 0,
        "flaky": false,
      }
    ]
  }
}
EOF

diff "$TMP/output_filtered.json" "$TMP/expected.json"
