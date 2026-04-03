#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "type": "string"
}
EOF

"$1" inspect "$TMP/schema.json" > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com
    Type              : Static
    Root              : https://example.com
    Pointer           :
    File Position     : 1:1
    Base              : https://example.com
    Relative Pointer  :
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com#/$id
    Type              : Static
    Root              : https://example.com
    Pointer           : /$id
    File Position     : 5:3
    Base              : https://example.com
    Relative Pointer  : /$id
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com#/$schema
    Type              : Static
    Root              : https://example.com
    Pointer           : /$schema
    File Position     : 2:3
    Base              : https://example.com
    Relative Pointer  : /$schema
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com#/description
    Type              : Static
    Root              : https://example.com
    Pointer           : /description
    File Position     : 4:3
    Base              : https://example.com
    Relative Pointer  : /description
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com#/title
    Type              : Static
    Root              : https://example.com
    Pointer           : /title
    File Position     : 3:3
    Base              : https://example.com
    Relative Pointer  : /title
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com#/type
    Type              : Static
    Root              : https://example.com
    Pointer           : /type
    File Position     : 6:3
    Base              : https://example.com
    Relative Pointer  : /type
    Dialect           : http://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    File Position     : 2:3
    Destination       : http://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : http://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
