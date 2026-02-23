#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "https://example.com/my-schema"
}
EOF

cat << 'EOF' > "$TMP/zz-meta.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/my-meta",
  "$vocabulary": {
    "https://json-schema.org/draft/2020-12/vocab/core": true,
    "https://json-schema.org/draft/2020-12/vocab/applicator": true,
    "https://json-schema.org/draft/2020-12/vocab/validation": true
  }
}
EOF

cat << 'EOF' > "$TMP/aa-schema.json"
{
  "$schema": "https://example.com/my-meta",
  "$id": "https://example.com/my-schema",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
"hello"
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/zz-meta.json" \
  --resolve "$TMP/aa-schema.json" --debug 2>"$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
debug: Detecting schema resources from file: $(realpath "$TMP")/zz-meta.json
debug: Importing schema into the resolution context: file://$(realpath "$TMP")/zz-meta.json
debug: Importing schema into the resolution context: https://example.com/my-meta
debug: Detecting schema resources from file: $(realpath "$TMP")/aa-schema.json
debug: Importing schema into the resolution context: file://$(realpath "$TMP")/aa-schema.json
debug: Importing schema into the resolution context: https://example.com/my-schema
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
