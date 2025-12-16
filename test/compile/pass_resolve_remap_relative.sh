#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "other"
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/nested",
  "title": "Test",
  "description": "Test schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "resolve": {
    "https://example.com/other": "https://example.com/middle",
    "https://example.com/middle": "https://example.com/nested"
  }
}
EOF

cd "$TMP"

"$1" compile schema.json \
  --resolve remote.json --verbose > "$TMP/template.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
[
  false,
  true,
  [ "", "https://example.com", "https://example.com/other" ],
  [
    [
      51,
      "/$ref",
      "",
      "#/$ref",
      2,
      [ 0 ],
      [
        [
          44,
          "/description",
          "",
          "#/description",
          3,
          [ 1, "Test schema" ]
        ],
        [
          44,
          "/title",
          "",
          "#/title",
          3,
          [ 1, "Test" ]
        ],
        [
          11,
          "/type",
          "",
          "#/type",
          3,
          [ 8, 4 ]
        ]
      ]
    ],
    [
      44,
      "/description",
      "",
      "#/description",
      2,
      [ 1, "Test schema" ]
    ],
    [
      44,
      "/title",
      "",
      "#/title",
      2,
      [ 1, "Test" ]
    ]
  ]
]
EOF

diff "$TMP/template.json" "$TMP/expected.json"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Detecting schema resources from file: $(realpath "$TMP")/remote.json
Importing schema into the resolution context: file://$(realpath "$TMP")/remote.json
Importing schema into the resolution context: https://example.com/nested
Resolving https://example.com/other as https://example.com/middle given the configuration file
Resolving https://example.com/middle as https://example.com/nested given the configuration file
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
