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
    File Position     : 1:1
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$defs
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs
    File Position     : 6:3
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(SUBSCHEMA) URI: file://$(realpath "$TMP")/schema.json#/\$defs/string
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs/string
    File Position     : 7:5
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs/string
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : yes

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$defs/string/type
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$defs/string/type
    File Position     : 7:17
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$defs/string/type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /\$defs/string
    Property Name     : no
    Orphan            : yes

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$ref
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$ref
    File Position     : 5:3
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/\$schema
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /\$schema
    File Position     : 2:3
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /\$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/description
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /description
    File Position     : 4:3
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /description
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$(realpath "$TMP")/schema.json#/title
    Type              : Static
    Root              : file://$(realpath "$TMP")/schema.json
    Pointer           : /title
    File Position     : 3:3
    Base              : file://$(realpath "$TMP")/schema.json
    Relative Pointer  : /title
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /\$ref
    Type              : Static
    File Position     : 5:3
    Destination       : file://$(realpath "$TMP")/schema.json#/\$defs/string
    - (w/o fragment)  : file://$(realpath "$TMP")/schema.json
    - (fragment)      : /\$defs/string

(REFERENCE) ORIGIN: /\$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
