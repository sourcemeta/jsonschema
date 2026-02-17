#!/bin/sh
set -e
cat << 'EOF' | "$1" metaschema -
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "string"
}
EOF
