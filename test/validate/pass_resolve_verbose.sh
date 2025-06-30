#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "id": "https://example.com",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "properties": {
    "foo": { "$ref": "foo" },
    "bar": { "$ref": "bar" }
  }
}
EOF

cat << 'EOF' > "$TMP/foo.json"
{
  "id": "https://example.com/foo",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "integer"
}
EOF

mkdir "$TMP/schemas"
cat << 'EOF' > "$TMP/schemas/baz.json"
{
  "id": "https://example.com/baz",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "array"
}
EOF

cat << 'EOF' > "$TMP/schemas/ignore-me.txt"
Foo Bar
EOF

mkdir "$TMP/schemas/nested"
cat << 'EOF' > "$TMP/schemas/nested/bar.json"
{
  "id": "https://example.com/bar",
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": 1 }
EOF

"$1" validate "$TMP/schema.json" "$TMP/instance.json" \
  --resolve "$TMP/foo.json" --resolve "$TMP/schemas" --verbose 2> "$TMP/stderr.txt"

cat << EOF > "$TMP/expected.txt"
Detecting schema resources from file: $(realpath "$TMP")/foo.json
Importing schema into the resolution context: file://$(realpath "$TMP")/foo.json
Importing schema into the resolution context: https://example.com/foo
Detecting schema resources from file: $(realpath "$TMP")/schemas/baz.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/baz.json
Importing schema into the resolution context: https://example.com/baz
Detecting schema resources from file: $(realpath "$TMP")/schemas/nested/bar.json
Importing schema into the resolution context: file://$(realpath "$TMP")/schemas/nested/bar.json
Importing schema into the resolution context: https://example.com/bar
ok: $(realpath "$TMP")/instance.json
  matches $(realpath "$TMP")/schema.json
EOF

diff "$TMP/stderr.txt" "$TMP/expected.txt"
