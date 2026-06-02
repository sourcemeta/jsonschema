#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string",
  "format": "email"
}
EOF

"$1" compile "$TMP/schema.json" --format-assertion \
  > "$TMP/template.json" 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << EOF > "$TMP/expected.json"
[
  5,
  false,
  true,
  [
    [
      [
        40,
        [ "format" ],
        [],
        "file://$(realpath "$TMP")/schema.json#/format",
        1,
        [ 14, 5 ]
      ],
      [
        96,
        [],
        [],
        "file://$(realpath "$TMP")/schema.json#/format",
        1,
        [ 8, 4 ],
        [
          [
            50,
            [ "format" ],
            [],
            "file://$(realpath "$TMP")/schema.json#/format",
            1,
            [ 1, "email" ]
          ]
        ]
      ],
      [
        11,
        [ "type" ],
        [],
        "file://$(realpath "$TMP")/schema.json#/type",
        1,
        [ 8, 4 ]
      ]
    ]
  ],
  []
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"
