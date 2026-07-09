#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
true
EOF

"$1" inspect "$TMP/schema.json" \
  --default-dialect "https://json-schema.org/draft/2020-12/schema" > "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
(RESOURCE) URI: file://$(realpath "$TMP")/schema.json
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           :
    File Position     : 1:1
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
