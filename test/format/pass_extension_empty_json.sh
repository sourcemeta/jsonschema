#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema1"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "description": "Test schema",
  "additionalProperties": false,
  "title": "Hello World"
}
EOF

cat << 'EOF' > "$TMP/schemas/schema2"
{"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "string", "title": "My String", "description": "Test schema"}
EOF

cat << 'EOF' > "$TMP/schemas/ignored.json"
{"$schema": "https://json-schema.org/draft/2020-12/schema", "type": "number", "title": "Should not be touched"}
EOF

cp "$TMP/schemas/ignored.json" "$TMP/schemas/ignored_original.json"

"$1" fmt "$TMP/schemas" --extension '' --verbose >"$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected_output.txt"
warning: Matching files with no extension
Formatting: $(realpath "$TMP")/schemas/schema1
Formatting: $(realpath "$TMP")/schemas/schema2
EOF

diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected_1.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Hello World",
  "description": "Test schema",
  "additionalProperties": false
}
EOF

cat << 'EOF' > "$TMP/expected_2.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "My String",
  "description": "Test schema",
  "type": "string"
}
EOF

diff "$TMP/schemas/schema1" "$TMP/expected_1.json"
diff "$TMP/schemas/schema2" "$TMP/expected_2.json"
diff "$TMP/schemas/ignored.json" "$TMP/schemas/ignored_original.json"
