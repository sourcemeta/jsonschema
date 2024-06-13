#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/document.json"
[ { "foo": 1 } ]
EOF

"$1" metaschema "$TMP/document.json" 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
fi

cat << EOF > "$TMP/expected.txt"
Not a schema: $(realpath "$TMP/document.json")
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
echo "PASS" 1>&2
