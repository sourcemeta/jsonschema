#!/bin/sh

set -o errexit
set -o nounset

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

VERSION="$(tr -d '[:space:]' < "$ROOT_DIR/VERSION")"
echo "VERSION file: $VERSION"

check_version() {
  test "$VERSION" = "$2" \
    || (echo "FAIL: $1 ($2) does not match VERSION ($VERSION)" 1>&2 && exit 1)
  echo "$1: $2"
}

check_contains() {
  grep -q "$2" "$3" \
    || (echo "FAIL: $1 does not contain '$2'" 1>&2 && exit 1)
  echo "$1: OK"
}

check_version "jsonschema --version" "$("$1" --version)"
check_version "jsonschema -v" "$("$1" -v)"
check_version "jsonschema version" "$("$1" version)"
check_version "package.json" "$(jq --raw-output '.version' "$ROOT_DIR/package.json")"
check_version "package-lock.json .version" "$(jq --raw-output '.version' "$ROOT_DIR/package-lock.json")"
check_version "package-lock.json .packages[\"\"].version" "$(jq --raw-output '.packages[""].version' "$ROOT_DIR/package-lock.json")"
check_contains "action.yml" "/install.sh\" $VERSION " "$ROOT_DIR/action.yml"
check_contains "README.markdown" "uses: sourcemeta/jsonschema@v$VERSION" "$ROOT_DIR/README.markdown"
