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
# Should fail with duplicate stdin error in JSON format
if echo '"foo"' | "$1" validate "$TMP/schema.json" - - --json > "$TMP/output.json" 2>&1; then
    echo "Validation succeeded unexpectedly"
    exit 1
fi

# Check if output is valid JSON and contains error message
if ! grep -q "Cannot read from standard input more than once" "$TMP/output.json"; then
    echo "JSON output does not contain expected error message"
    cat "$TMP/output.json"
    exit 1
fi
