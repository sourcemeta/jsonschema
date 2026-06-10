#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mkdir -p "$TMP/meta"

cat << 'EOF' > "$TMP/meta/custom.json"
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

cat << 'EOF' > "$TMP/schema.json"
{ "$id": "https://example.com/x", "type": "string" }
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "./meta/custom"
}
EOF

"$1" inspect "$TMP/schema.json" > "$TMP/output.txt"

cat << EOF > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com/x
    Type              : Static
    Root              : https://example.com/x
    Pointer           :
    File Position     : 1:1
    Base              : https://example.com/x
    Relative Pointer  :
    Dialect           : file://$(realpath "$TMP")/meta/custom.json
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/x#/\$id
    Type              : Static
    Root              : https://example.com/x
    Pointer           : /\$id
    File Position     : 1:3
    Base              : https://example.com/x
    Relative Pointer  : /\$id
    Dialect           : file://$(realpath "$TMP")/meta/custom.json
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/x#/type
    Type              : Static
    Root              : https://example.com/x
    Pointer           : /type
    File Position     : 1:35
    Base              : https://example.com/x
    Relative Pointer  : /type
    Dialect           : file://$(realpath "$TMP")/meta/custom.json
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
