#!/bin/sh
set -e
TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
# Should fail with duplicate stdin error
echo '"foo"' | "$1" validate "$TMP/schema.json" - - 2>&1 | grep "Cannot read from standard input more than once"
