#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "properties": {
    "foo": {
      "type": 1
    }
  }
}
EOF

"$1" validate "$TMP/schema.json" --metaschema && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
else
  echo "PASS" 1>&2
fi
