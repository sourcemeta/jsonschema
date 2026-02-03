#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema1.json"
{
  "title": "Schema 1",
  "description": "First schema",
  "examples": [ "foo" ],
  "type": "string",
  "$schema": "http://json-schema.org/draft-06/schema#",
  "const": "foo"
}
EOF

cat << 'EOF' > "$TMP/schemas/schema2.json"
{
  "title": "Schema 2",
  "description": "Second schema",
  "examples": [ 42 ],
  "type": "number",
  "$schema": "http://json-schema.org/draft-06/schema#"
}
EOF

"$1" lint "$TMP/schemas" --fix --format > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected_output.txt"
.
EOF
diff "$TMP/output.txt" "$TMP/expected_output.txt"

cat << 'EOF' > "$TMP/expected1.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Schema 1",
  "description": "First schema",
  "examples": [ "foo" ],
  "const": "foo"
}
EOF

cat << 'EOF' > "$TMP/expected2.json"
{
  "$schema": "http://json-schema.org/draft-06/schema#",
  "title": "Schema 2",
  "description": "Second schema",
  "examples": [ 42 ],
  "type": "number"
}
EOF

diff "$TMP/schemas/schema1.json" "$TMP/expected1.json"
diff "$TMP/schemas/schema2.json" "$TMP/expected2.json"
