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
OUTPUT=$(echo '{"foo": 123}' | "$1" validate "$TMP/schema.json" - 2>&1)
EXIT_CODE=$?
set -o errexit

if [ "$EXIT_CODE" -eq 0 ]; then
    echo "FAIL: Expected validation failure for type mismatch"
    exit 1
fi

set +o errexit
OUTPUT=$(cat "$TMP/schema.json" | "$1" validate - "$TMP/schema.json" 2>&1)
EXIT_CODE=$?
set -o errexit

if [ "$EXIT_CODE" -eq 0 ]; then
    echo "FAIL: Expected failure when passing schema as stdin"
    exit 1
fi

set +o errexit
OUTPUT=$(echo '{}' | "$1" validate "$TMP/schema.json" - - 2>&1)
EXIT_CODE=$?
set -o errexit

if [ "$EXIT_CODE" -eq 0 ]; then
    echo "FAIL: Expected failure for multiple stdin arguments"
    exit 1
fi

echo "PASS: All stdin tests passed."
