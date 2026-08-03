#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "x": {
      "allOf": [
        { "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#date" },
        { "x-jsonld-datatype": "http://www.w3.org/2001/XMLSchema#string" }
      ]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/test.json"
{
  "target": "https://example.com",
  "tests": [
    {
      "description": "Conflicting datatypes",
      "valid": true,
      "data": { "x": "v" },
      "rdf": []
    }
  ]
}
EOF

"$1" test "$TMP/test.json" --resolve "$TMP/schema.json" --json > "$TMP/output.json" 2>&1 \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
# Test assertion failure
test "$EXIT_CODE" = "2"

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
        "name": "Conflicting datatypes",
        "status": "failed",
        "suite": [ "https://example.com" ],
        "type": "unit",
        "filePath": "$(realpath "$TMP")/test.json",
        "line": 4,
        "retries": 0,
        "flaky": false,
        "trace": "error: A JSON-LD datatype cannot be assigned more than one value\n  at instance location \"/x\"\n  at facet \"datatype\"\n  at schema location https://example.com#/properties/x/allOf/0/x-jsonld-datatype\n  at conflicting schema location https://example.com#/properties/x/allOf/1/x-jsonld-datatype\n"
      }
    ]
  }
}
EOF

diff "$TMP/output_filtered.json" "$TMP/expected.json"
