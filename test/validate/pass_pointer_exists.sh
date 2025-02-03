#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "nested": {
      "type": "object",
      "properties": {
        "foo": { "type": "integer" }
      },
      "required": ["foo"]
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{
  "foo": 123
}
EOF

if ! "$1" validate \
    "$TMP/schema.json" \
    "$TMP/instance.json" \
    --path=/properties/nested \
    1>/dev/null 2>"$TMP/stderr.log"
then
  echo "Expected success, got failure!"
  cat "$TMP/stderr.log"
  exit 1
fi

echo "pass_pointer_exists.sh: PASS"
