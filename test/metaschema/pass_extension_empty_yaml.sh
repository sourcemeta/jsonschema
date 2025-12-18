#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir "$TMP/schemas"

cat << 'EOF' > "$TMP/schemas/schema1"
$schema: http://json-schema.org/draft-04/schema#
title: Test
description: Test schema
type: string
EOF

cat << 'EOF' > "$TMP/schemas/schema2"
$schema: http://json-schema.org/draft-04/schema#
title: Test 2
description: Test schema 2
type: object
EOF

cat << 'EOF' > "$TMP/schemas/ignored.yaml"
$schema: http://json-schema.org/draft-04/schema#
title: Ignored
description: This file should be ignored
type: boolean
EOF

"$1" metaschema "$TMP/schemas" --extension '' --verbose > "$TMP/output.txt" 2>&1

cat << EOF > "$TMP/expected.txt"
warning: Matching files with no extension
ok: $(realpath "$TMP")/schemas/schema1
  matches http://json-schema.org/draft-04/schema#
ok: $(realpath "$TMP")/schemas/schema2
  matches http://json-schema.org/draft-04/schema#
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
