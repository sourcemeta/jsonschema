#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
FUSE_MOUNT="$(mktemp -d)"
clean() {
  fusermount3 -u "$FUSE_MOUNT" || fusermount -u "$FUSE_MOUNT" || true
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

( cd "$FUSE_MOUNT" && "$1" validate --verbose \
  level1/level2/level3/schema.json instance.json ) > "$TMP/output.txt" 2>&1

cat << 'EOF' > "$TMP/expected.txt"
ok: instance.json

1 validated, 1 passed, 0 failed
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
