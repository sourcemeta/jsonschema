#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" --trace > "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
-> (push) "/properties/foo/type"
   at "/foo"
<- (pass) "/properties/foo/type"
   at "/foo"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
