#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

cat << 'EOF' > "$TMP/jsonschema.json"
{
  "defaultDialect": "https://json-schema.org/draft/2020-12/schema"
}
EOF

cd "$TMP"

"$1" inspect schema.json --verbose > "$TMP/result.txt" 2> "$TMP/output.txt"

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
    File Position     : 4:3
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
    File Position     : 5:5
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
    File Position     : 5:17
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
    File Position     : 2:3
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
    File Position     : 3:3
    Base              : https://example.com
    Relative Pointer  : /$ref
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no

(REFERENCE) ORIGIN: /$ref
    Type              : Static
    File Position     : 3:3
    Destination       : https://example.com#/$defs/string
    - (w/o fragment)  : https://example.com
    - (fragment)      : /$defs/string
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"

cat << 'EOF' > "$TMP/expected_log.txt"
EOF

diff "$TMP/output.txt" "$TMP/expected_log.txt"
