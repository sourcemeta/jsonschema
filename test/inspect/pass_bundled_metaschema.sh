#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "https://example.com/meta",
  "$id": "https://example.com/schema",
  "type": "string",
  "$defs": {
    "https://example.com/meta": {
      "$schema": "https://json-schema.org/draft/2020-12/schema",
      "$id": "https://example.com/meta",
      "$vocabulary": {
        "https://json-schema.org/draft/2020-12/vocab/core": true,
        "https://json-schema.org/draft/2020-12/vocab/validation": true
      },
      "type": "object"
    }
  }
}
EOF

"$1" inspect "$TMP/schema.json" > "$TMP/result.txt" 2> "$TMP/stderr.txt"

cat << 'EOF' > "$TMP/expected_stderr.txt"
EOF

diff "$TMP/stderr.txt" "$TMP/expected_stderr.txt"

cat << 'EOF' > "$TMP/expected.txt"
(RESOURCE) URI: https://example.com/meta
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta
    File Position     : 6:5
    Base              : https://example.com/meta
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/$id
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$id
    File Position     : 8:7
    Base              : https://example.com/meta
    Relative Pointer  : /$id
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/$schema
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$schema
    File Position     : 7:7
    Base              : https://example.com/meta
    Relative Pointer  : /$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/$vocabulary
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary
    File Position     : 9:7
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    File Position     : 10:9
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    File Position     : 11:9
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/meta#/type
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/type
    File Position     : 13:7
    Base              : https://example.com/meta
    Relative Pointer  : /type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(RESOURCE) URI: https://example.com/schema
    Type              : Static
    Root              : https://example.com/schema
    Pointer           :
    File Position     : 1:1
    Base              : https://example.com/schema
    Relative Pointer  :
    Dialect           : https://example.com/meta
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/schema#/$defs
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs
    File Position     : 5:3
    Base              : https://example.com/schema
    Relative Pointer  : /$defs
    Dialect           : https://example.com/meta
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(SUBSCHEMA) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta
    File Position     : 6:5
    Base              : https://example.com/meta
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/$id
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$id
    File Position     : 8:7
    Base              : https://example.com/meta
    Relative Pointer  : /$id
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/$schema
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$schema
    File Position     : 7:7
    Base              : https://example.com/meta
    Relative Pointer  : /$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/$vocabulary
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary
    File Position     : 9:7
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    File Position     : 10:9
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1core
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    File Position     : 11:9
    Base              : https://example.com/meta
    Relative Pointer  : /$vocabulary/https:~1~1json-schema.org~1draft~12020-12~1vocab~1validation
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$defs/https:~1~1example.com~1meta/type
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$defs/https:~1~1example.com~1meta/type
    File Position     : 13:7
    Base              : https://example.com/meta
    Relative Pointer  : /type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : /$defs/https:~1~1example.com~1meta
    Property Name     : no
    Orphan            : yes

(POINTER) URI: https://example.com/schema#/$id
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$id
    File Position     : 3:3
    Base              : https://example.com/schema
    Relative Pointer  : /$id
    Dialect           : https://example.com/meta
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/schema#/$schema
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /$schema
    File Position     : 2:3
    Base              : https://example.com/schema
    Relative Pointer  : /$schema
    Dialect           : https://example.com/meta
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: https://example.com/schema#/type
    Type              : Static
    Root              : https://example.com/schema
    Pointer           : /type
    File Position     : 4:3
    Base              : https://example.com/schema
    Relative Pointer  : /type
    Dialect           : https://example.com/meta
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /$defs/https:~1~1example.com~1meta/$schema
    Type              : Static
    File Position     : 7:7
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>

(REFERENCE) ORIGIN: /$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://example.com/meta
    - (w/o fragment)  : https://example.com/meta
    - (fragment)      : <NONE>
EOF

diff "$TMP/result.txt" "$TMP/expected.txt"
