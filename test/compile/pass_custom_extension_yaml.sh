#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.custom"
$id: https://example.com
$schema: https://json-schema.org/draft/2020-12/schema
type: string
EOF

"$1" compile "$TMP/schema.custom" > "$TMP/result.json"

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  true,
  [ "", "https://example.com" ],
  [
    [
      11,
      "/type",
      "",
      "#/type",
      2,
      [ 8, 4 ]
    ]
  ]
]
EOF

diff "$TMP/result.json" "$TMP/expected.json"
