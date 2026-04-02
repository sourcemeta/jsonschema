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
  "title": "Test",
  "description": "Test schema",
  "additionalProperties": {
    "type": "string"
  }
}
EOF

"$1" compile "$TMP/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json" > "$TMP/template.json"

cat << EOF > "$TMP/expected.json"
[
  1,
  false,
  true,
  [
    [
      [
        44,
        [ "description" ],
        [],
        "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json#/description",
        1,
        [ 1, "Test schema" ]
      ],
      [
        44,
        [ "title" ],
        [],
        "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json#/title",
        1,
        [ 1, "Test" ]
      ],
      [
        61,
        [ "additionalProperties" ],
        [],
        "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json#/additionalProperties",
        1,
        [ 0 ],
        [
          [
            11,
            [ "type" ],
            [],
            "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json#/additionalProperties/type",
            1,
            [ 8, 4 ]
          ],
          [
            46,
            [],
            [],
            "file://$(realpath "$TMP")/foo/bar/baz/qux/very/long/path/foo/bar/baz/qux/schema.json#/additionalProperties",
            1,
            [ 0 ]
          ]
        ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
