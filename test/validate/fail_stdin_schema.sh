#!/bin/sh
set -e
TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

echo '{}' > "$TMP/instance.json"

cat << 'EOF' > "$TMP/schema_input.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object"
}
EOF

# Expected to fail reading schema '-'
if "$1" validate - "$TMP/instance.json" < "$TMP/schema_input.json" >/dev/null 2>&1; then
  echo "Validation succeeded unexpectedly (schema from stdin not supported yet)"
  exit 1
fi
