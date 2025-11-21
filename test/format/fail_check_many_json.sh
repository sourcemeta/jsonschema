#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/1.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

cat << 'EOF' > "$TMP/schemas/2.json"
{
  "type": 1,
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

cat << 'EOF' > "$TMP/schemas/3.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#"
}
EOF

"$1" fmt "$TMP/schemas" --check --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "errors": [
    "$(realpath "$TMP")/schemas/1.json",
    "$(realpath "$TMP")/schemas/2.json"
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
