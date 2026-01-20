#!/bin/bash

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "foo": { "type": "string" }
  }
}
EOF

echo '{"foo": "bar"}' | "$1" validate "$TMP/schema.json" -

echo 'foo: bar' | "$1" validate "$TMP/schema.json" -

set +o errexit
cat "$TMP/schema.json" > "$TMP/schema_stdin.json"

OUTPUT=$("$1" validate - "$TMP/schema.json" < "$TMP/schema_stdin.json" 2>&1)

EXIT_CODE=$?
set -o errexit

: "$OUTPUT"

if [ "$EXIT_CODE" -eq 0 ]; then
  echo "FAIL: Expected failure when passing schema via stdin"
  echo "$OUTPUT"
  exit 1
fi

if ! echo "$OUTPUT" | grep -q "Reading schema from stdin is not supported"; then
  echo "FAIL: Expected schema-from-stdin error message"
  echo "$OUTPUT"
  exit 1
fi

set +o errexit
OUTPUT=$("$1" validate - - < "$TMP/schema.json" 2>&1)
EXIT_CODE=$?
set -o errexit

: "$OUTPUT"

if [ "$EXIT_CODE" -eq 0 ]; then
  echo "FAIL: Expected failure when passing multiple stdin inputs"
  echo "$OUTPUT"
  exit 1
fi

echo "PASS: stdin validation tests passed."
