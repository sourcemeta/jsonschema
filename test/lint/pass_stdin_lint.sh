#!/bin/sh
set -e
cat << 'EOF' | "$1" lint -
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Test",
  "description": "Test description",
  "examples": ["foo"],
  "type": "string"
}
EOF
