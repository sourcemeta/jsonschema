#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.yaml"
$schema: https://json-schema.org/draft/2020-12/schema
$id: https://example.com
$ref: '#/$defs/string'
$defs:
  string: { type: string }
EOF

"$1" inspect "$TMP/schema.yaml" > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com
    Type              : Static
    Root              : https://example.com
    Pointer           :
    Base              : https://example.com
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Instance Location :

(POINTER) URI: https://example.com#/$defs
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs
    Base              : https://example.com
    Relative Pointer  : /$defs
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(SUBSCHEMA) URI: https://example.com#/$defs/string
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs/string
    Base              : https://example.com
    Relative Pointer  : /$defs/string
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Instance Location :

(POINTER) URI: https://example.com#/$defs/string/type
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs/string/type
    Base              : https://example.com
    Relative Pointer  : /$defs/string/type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/string

(POINTER) URI: https://example.com#/$id
    Type              : Static
    Root              : https://example.com
    Pointer           : /$id
    Base              : https://example.com
    Relative Pointer  : /$id
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(POINTER) URI: https://example.com#/$ref
    Type              : Static
    Root              : https://example.com
    Pointer           : /$ref
    Base              : https://example.com
    Relative Pointer  : /$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(POINTER) URI: https://example.com#/$schema
    Type              : Static
    Root              : https://example.com
    Pointer           : /$schema
    Base              : https://example.com
    Relative Pointer  : /$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :

(REFERENCE) ORIGIN: /$ref
    Type              : Static
    Destination       : https://example.com#/$defs/string
    - (w/o fragment)  : https://example.com
    - (fragment)      : /$defs/string

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
