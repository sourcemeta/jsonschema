#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
FUSE_MOUNT="$(mktemp -d)"
clean() {
  fusermount -u "$FUSE_MOUNT" 2>/dev/null || true
  rm -rf "$TMP" "$FUSE_MOUNT"
}
trap clean EXIT

mkdir -p "$TMP/level1/level2/level3"

cat << 'EOF' > "$TMP/level1/level2/level3/schema.json"
{
  "properties": {
    "foo": {
      "type": "string"
    }
  }
}
EOF

cat << 'EOF' > "$TMP/instance.json"
{ "foo": "bar" }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

bindfs --no-allow-other "$TMP" "$FUSE_MOUNT"

"$1" validate --verbose "$FUSE_MOUNT/level1/level2/level3/schema.json" "$FUSE_MOUNT/instance.json" 2> "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
ok: $(realpath "$FUSE_MOUNT")/instance.json
  matches $(realpath "$FUSE_MOUNT")/level1/level2/level3/schema.json
annotation: "foo"
  at instance location "" (line 1, column 1)
  at evaluate path "/properties"
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
