#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft-07/schema#",
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
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
    Dialect           : https://json-schema.org/draft-07/schema#
    Base Dialect      : http://json-schema.org/draft-07/schema#
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://json-schema.org/draft-07/schema
    - (w/o fragment)  : https://json-schema.org/draft-07/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
