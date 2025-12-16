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
  "$ref": "nested"
}
EOF

mkdir "$TMP/schemas"
cat << 'EOF' > "$TMP/schemas/remote.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com/nested",
  "type": "string"
}
EOF

"$1" bundle "$TMP/schema.json" --resolve "$TMP/schemas" --without-id > "$TMP/result.json" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$ref": "#/$defs/https:~1~1example.com~1nested",
  "$defs": {
    "https://example.com/nested": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "title": "Test",
      "description": "Test schema",
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/expected-stderr.txt"
warning: You are opting in to remove schema identifiers in the bundled schema.
The only legit use case of this advanced feature we know of is to workaround
non-compliant JSON Schema implementations such as Visual Studio Code.
Otherwise, this is not needed and may harm other use cases. For example,
you will be unable to reference the resulting schema from other schemas
using the --resolve/-r option.
EOF

diff "$TMP/result.json" "$TMP/expected.json"
diff "$TMP/stderr.txt" "$TMP/expected-stderr.txt"

# Must come out formatted
"$1" fmt "$TMP/result.json" --check
