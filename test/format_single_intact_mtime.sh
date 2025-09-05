#!/bin/sh

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

mtime() {
  if stat -c %Y "$1" 2>/dev/null
  then
    return
  else
    stat -f %m "$1" 2>/dev/null
  fi
}

cat << 'EOF' > "$TMP/schema.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

mtime_old="$(mtime "$TMP/schema.json")"
sleep 1

"$1" fmt "$TMP/schema.json"
mtime_new="$(mtime "$TMP/schema.json")"

if [ "$mtime_old" != "$mtime_new" ]
then
  echo "The file was unexpectedly modified" 1>&2
  exit 1
fi

cat << 'EOF' > "$TMP/expected.json"
{
  "$schema": "http://json-schema.org/draft-04/schema#",
  "type": "string"
}
EOF

diff "$TMP/schema.json" "$TMP/expected.json"
