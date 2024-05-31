#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
# shellcheck disable=SC2317
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string",
  "enum": [ "foo" ]
}
EOF

"$1" lint "$TMP/schema.json" && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
else
  echo "PASS" 1>&2
  exit 0
fi
