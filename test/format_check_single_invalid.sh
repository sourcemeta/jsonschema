#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#"
  "type" 1,
}
EOF

"$1" fmt "$TMP/schema.json" --check 2>"$TMP/output.txt" && CODE="$?" || CODE="$?"

if [ "$CODE" = "0" ]
then
  echo "FAIL" 1>&2
  exit 1
fi

cat << EOF > "$TMP/expected.txt"
$(realpath "$TMP/schema.json")
  Failed to parse the JSON document at line 3 and column 3
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
echo "PASS" 1>&2
