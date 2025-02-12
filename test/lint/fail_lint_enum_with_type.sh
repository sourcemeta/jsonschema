#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "enum": [ "foo", "bar" ]
}
EOF

if "$1" lint "$TMP/schema.json" > "$TMP/stderr.txt" 2>&1; then
  echo "Expected 'lint' to fail, but it succeeded!"
  exit 1
fi

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP")/schema.json:
  Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types (enum_with_type)
    at schema location ""
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
