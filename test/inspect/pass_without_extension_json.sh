#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" inspect "$TMP/schema" > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com
    Type              : Static
    Root              : https://example.com
    Pointer           :
    File Position     : 1:1
    Base              : https://example.com
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no

(POINTER) URI: https://example.com#/$defs
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs
    File Position     : 7:3
    Base              : https://example.com
    Relative Pointer  : /$defs
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(SUBSCHEMA) URI: https://example.com#/$defs/string
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs/string
    File Position     : 8:5
    Base              : https://example.com
    Relative Pointer  : /$defs/string
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(POINTER) URI: https://example.com#/$defs/string/type
    Type              : Static
    Root              : https://example.com
    Pointer           : /$defs/string/type
    File Position     : 8:17
    Base              : https://example.com
    Relative Pointer  : /$defs/string/type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/string
    Property Name     : no

(POINTER) URI: https://example.com#/$id
    Type              : Static
    Root              : https://example.com
    Pointer           : /$id
    File Position     : 5:3
    Base              : https://example.com
    Relative Pointer  : /$id
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(POINTER) URI: https://example.com#/$ref
    Type              : Static
    Root              : https://example.com
    Pointer           : /$ref
    File Position     : 6:3
    Base              : https://example.com
    Relative Pointer  : /$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(POINTER) URI: https://example.com#/$schema
    Type              : Static
    Root              : https://example.com
    Pointer           : /$schema
    File Position     : 2:3
    Base              : https://example.com
    Relative Pointer  : /$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(POINTER) URI: https://example.com#/description
    Type              : Static
    Root              : https://example.com
    Pointer           : /description
    File Position     : 4:3
    Base              : https://example.com
    Relative Pointer  : /description
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(POINTER) URI: https://example.com#/title
    Type              : Static
    Root              : https://example.com
    Pointer           : /title
    File Position     : 3:3
    Base              : https://example.com
    Relative Pointer  : /title
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(REFERENCE) ORIGIN: /$ref
    Type              : Static
    File Position     : 6:3
    Destination       : https://example.com#/$defs/string
    - (w/o fragment)  : https://example.com
    - (fragment)      : /$defs/string

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
