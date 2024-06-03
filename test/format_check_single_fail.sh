#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

"$1" fmt "$TMP/schema.json" --check && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
else
  echo "PASS" 1>&2
fi

cat << 'EOF' > "$TMP/expected.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
