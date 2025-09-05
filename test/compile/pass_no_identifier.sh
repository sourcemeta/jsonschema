#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux"

cat << 'EOF' > "$TMP/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json" > "$TMP/template.json"

cat << EOF > "$TMP/expected.json"
[
  false,
  true,
  [
    "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json"
  ],
  [
    [
      61,
      "/additionalProperties",
      "",
      "#/additionalProperties",
      1,
      [ 0 ],
      [
        [
          11,
          "/type",
          "",
          "#/additionalProperties/type",
          1,
          [ 8, 4 ]
        ],
        [
          46,
          "",
          "",
          "#/additionalProperties",
          1,
          [ 0 ]
        ]
      ]
    ]
  ]
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
