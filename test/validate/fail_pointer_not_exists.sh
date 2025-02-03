#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{
  "some": "data"
}
EOF

if "$1" validate \
    "$TMP/schema.json" \
    "$TMP/instance.json" \
    --path=/nope \
    1>/dev/null 2>"$TMP/stderr.log"
then
  echo "Expected failure, but got success!"
  exit 1
fi

if ! grep -q 'Failed to resolve JSON Pointer' "$TMP/stderr.log"; then
  echo "Didn't see 'Failed to resolve pointer' in the output!"
  cat "$TMP/stderr.log"
  exit 1
fi

echo "fail_pointer_not_exists.sh: PASS"
