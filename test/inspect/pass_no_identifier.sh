#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" inspect "$TMP/schema.json" > "$TMP/result.txt"

cat "$TMP/result.txt"

cat << EOF > "$TMP/expected.txt"
(RESOURCE) URI: file://$(realpath "$TMP")/schema.json
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           :
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Instance Location :

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$defs
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(SUBSCHEMA) URI: file://$(realpath "$TMP")/schema.json#/\$defs/string
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs/string
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs/string
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Instance Location :

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$defs/string/type
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs/string/type
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs/string/type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /\$defs/string

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$ref
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$ref
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$schema
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$schema
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(REFERENCE) ORIGIN: /\$ref
    Type              : Static
    Destination       : file://$(realpath "$TMP")/schema.json#/\$defs/string
    - (w/o fragment)  : file://$(realpath "$TMP")/schema.json
    - (fragment)      : /\$defs/string

(REFERENCE) ORIGIN: /\$schema
    Type              : Static
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
