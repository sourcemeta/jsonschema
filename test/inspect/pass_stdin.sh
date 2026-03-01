#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' | "$1" inspect - >"$TMP/output.txt" 2>&1
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com/test",
  "type": "string"
}
EOF

cat << 'EOF' > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com/test
    Type              : Static
    Root              : https://example.com/test
    Pointer           :
    File Position     : 1:1
    Base              : https://example.com/test
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/test#/$id
    Type              : Static
    Root              : https://example.com/test
    Pointer           : /$id
    File Position     : 3:3
    Base              : https://example.com/test
    Relative Pointer  : /$id
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/test#/$schema
    Type              : Static
    Root              : https://example.com/test
    Pointer           : /$schema
    File Position     : 2:3
    Base              : https://example.com/test
    Relative Pointer  : /$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/test#/type
    Type              : Static
    Root              : https://example.com/test
    Pointer           : /type
    File Position     : 4:3
    Base              : https://example.com/test
    Relative Pointer  : /type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
