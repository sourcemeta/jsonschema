#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "https://example.com",
  "$ref": "#/$defs/string",
  "$defs": {
    "string": { "type": "string" }
  }
}
EOF

"$1" inspect "$TMP/schema.json" > "$TMP/result.txt"

cat << 'EOF' > "$TMP/expected.txt"
(LOCATION) URI: https://example.com
    Type             : Static
    Schema           : https://example.com
    Pointer          :
    Base URI         : https://example.com
    Relative Pointer :
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(POINTER) URI: https://example.com#/$defs
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$defs
    Base URI         : https://example.com
    Relative Pointer : /$defs
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(SUBSCHEMA) URI: https://example.com#/$defs/string
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$defs/string
    Base URI         : https://example.com
    Relative Pointer : /$defs/string
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(POINTER) URI: https://example.com#/$defs/string/type
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$defs/string/type
    Base URI         : https://example.com
    Relative Pointer : /$defs/string/type
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(POINTER) URI: https://example.com#/$id
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$id
    Base URI         : https://example.com
    Relative Pointer : /$id
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(POINTER) URI: https://example.com#/$ref
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$ref
    Base URI         : https://example.com
    Relative Pointer : /$ref
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(POINTER) URI: https://example.com#/$schema
    Type             : Static
    Schema           : https://example.com
    Pointer          : /$schema
    Base URI         : https://example.com
    Relative Pointer : /$schema
    Dialect          : https://json-schema.org/draft/2020-12/schema
    Base Dialect     : https://json-schema.org/draft/2020-12/schema
(REFERENCE) URI: /$ref
    Type             : Static
    Destination      : https://example.com#/$defs/string
    - (w/o fragment) : https://example.com
    - (fragment)     : /$defs/string
(REFERENCE) URI: /$schema
    Type             : Static
    Destination      : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment) : https://json-schema.org/draft/2020-12/schema
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
