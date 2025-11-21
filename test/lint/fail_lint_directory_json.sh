#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/foo.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "type": "string",
  "enum": [ "foo", "bar", "baz" ]
}
EOF

cat << 'EOF' > "$TMP/schemas/bar.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#"
}
EOF

"$1" lint "$TMP/schemas" --json >"$TMP/output.json" 2>&1 && CODE="$?" || CODE="$?"
test "$CODE" = "2" || exit 1

cat << EOF > "$TMP/expected.json"
{
  "valid": false,
  "health": 50,
  "errors": [
    {
      "path": "$(realpath "$TMP")/schemas/foo.json",
      "id": "enum_with_type",
      "message": "Setting \`type\` alongside \`enum\` is considered an anti-pattern, as the enumeration choices already imply their respective types",
      "description": null,
      "schemaLocation": "/enum",
      "position": [ 4, 3, 4, 33 ]
    }
  ]
}
EOF

diff "$TMP/output.json" "$TMP/expected.json"
