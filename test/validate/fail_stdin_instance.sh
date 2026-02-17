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
# Should fail validation (exit code 2)
if echo '123' | "$1" validate "$TMP/schema.json" -; then
  echo "Validation should have failed"
  exit 1
fi
