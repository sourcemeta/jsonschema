#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

CWD="$(pwd -P)"

cat << 'EOF' | "$1" inspect - > "$TMP/output.txt" 2>&1
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF

cat << EOF > "$TMP/expected.txt"
(RESOURCE) URI: file://$CWD
    Type              : Static
    Root              : file://$CWD
    Pointer           :
    File Position     : 1:1
    Base              : file://$CWD
    Relative Pointer  :
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            : <NONE>
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$CWD#/\$schema
    Type              : Static
    Root              : file://$CWD
    Pointer           : /\$schema
    File Position     : 2:3
    Base              : file://$CWD
    Relative Pointer  : /\$schema
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(POINTER) URI: file://$CWD#/type
    Type              : Static
    Root              : file://$CWD
    Pointer           : /type
    File Position     : 3:3
    Base              : file://$CWD
    Relative Pointer  : /type
    Dialect           : https://json-schema.org/draft/2020-12/schema
    Base Dialect      : https://json-schema.org/draft/2020-12/schema
    Parent            :
    Property Name     : no
    Orphan            : no

(REFERENCE) ORIGIN: /\$schema
    Type              : Static
    File Position     : 2:3
    Destination       : https://json-schema.org/draft/2020-12/schema
    - (w/o fragment)  : https://json-schema.org/draft/2020-12/schema
    - (fragment)      : <NONE>
EOF

diff "$TMP/output.txt" "$TMP/expected.txt"
