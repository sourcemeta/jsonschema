#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

"$1" fmt "$TMP/schema.json" --indentation 4 --check 2>"$TMP/stderr.txt" && CODE="$?" || CODE="$?"
test "$CODE" = "1" || exit 1

cat << EOF > "$TMP/error.txt"
FAIL: $(realpath "$TMP")/schema.json
Got:
{
  "\$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}

But expected:
{
    "\$schema": "http://json-schema.org/draft-04/schema#",
    "type": 1
}

EOF

diff "$TMP/stderr.txt" "$TMP/error.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": 1
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
