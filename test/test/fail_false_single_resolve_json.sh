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
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Should fail but passes",
      "valid": false,
      "data": "valid-string"
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2" || exit 1

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
      "tests": 1,
      "passed": 0,
      "failed": 1,
      "pending": 0,
      "skipped": 0,
      "other": 0,
    },
    "tests": [
      {
        "name": "Should fail but passes",
        "status": "failed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
        "message": "Passed but was expected to fail"
      }
    ]
  }
}
EOF

diff "$TMP/output_filtered.json" "$TMP/expected.json"
