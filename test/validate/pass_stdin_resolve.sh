#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/defs.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/defs",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

# Schema from stdin referencing a resolved external schema
cat << 'EOF' | "$1" validate - "$TMP/instance.json" --resolve "$TMP/defs.json" \
  && EXIT_CODE="$?" || EXIT_CODE="$?"
{"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"https://example.com/defs"}
EOF
test "$EXIT_CODE" = "0" || exit 1
