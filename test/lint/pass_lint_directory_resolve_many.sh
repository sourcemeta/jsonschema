#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/1.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "id": "https://example.com/1"
}
EOF

cat << 'EOF' > "$TMP/schemas/2.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "id": "https://example.com/2"
}
EOF

"$1" lint --resolve "$TMP/schemas" "$TMP/schemas" --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/schemas/1.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/1.json
Importing schema into the resolution context: https://example.com/1
Detecting schema resources from file: $(realpath "$TMP")/schemas/2.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/2.json
Importing schema into the resolution context: https://example.com/2
Linting: $(realpath "$TMP")/schemas/1.json
Linting: $(realpath "$TMP")/schemas/2.json
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
