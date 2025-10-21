#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "other"
}
EOF

cat << 'EOF' > "$TMP/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/nested",
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

"$1" bundle schema.json --resolve remote.json --verbose > "$TMP/result.json" 2> "$TMP/output.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "other",
  "$defs": {
    "https://example.com/other": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/other",
      "type": "string"
    }
  }
}
EOF

diff "$TMP/result.json" "$TMP/expected.json"

cat << EOF > "$TMP/expected.txt"
Using configuration file: $(realpath "$TMP")/jsonschema.json
Detecting schema resources from file: $(realpath "$TMP")/remote.json
Importing schema into the resolution context: file://$(realpath "$TMP")/remote.json
Importing schema into the resolution context: https://example.com/nested
Resolving https://example.com/other as https://example.com/middle given the configuration file
Resolving https://example.com/middle as https://example.com/nested given the configuration file
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
