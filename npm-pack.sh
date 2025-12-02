#!/bin/sh

set -o errexit
set -o nounset

if [ $# -lt 1 ]
then
  echo "Usage: $0 <directory>" 1>&2
  exit 1
fi

VERSION="$(jq '.version' --raw-output package.json | tr -d 'v')"
DIRECTORY="$1"
OUTPUT="$(pwd)/build"

# (1) Download artifacts
mkdir -p "$OUTPUT/npm/artifacts"
echo "Preparing $VERSION" 1>&2
PACKAGE_BASE_URL="https://github.com/sourcemeta/jsonschema/releases/download/v$VERSION"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/darwin-arm64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-arm64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/darwin-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/linux-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-linux-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/linux-arm64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-linux-arm64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/windows-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-windows-x86_64.zip"
unzip -o "$OUTPUT/npm/artifacts/darwin-arm64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/darwin-x86_64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/linux-x86_64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/linux-arm64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/windows-x86_64.zip" -d "$OUTPUT/npm/artifacts"
ls -l "$OUTPUT/npm/artifacts"

# (2) Stage package contents
rm -rf "$OUTPUT/github-releases"
mkdir -p "$OUTPUT/github-releases"

install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-darwin-arm64/bin/jsonschema" \
  "$OUTPUT/github-releases/jsonschema-darwin-arm64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-darwin-x86_64/bin/jsonschema" \
  "$OUTPUT/github-releases/jsonschema-darwin-x86_64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-linux-x86_64/bin/jsonschema" \
  "$OUTPUT/github-releases/jsonschema-linux-x86_64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-linux-arm64/bin/jsonschema" \
  "$OUTPUT/github-releases/jsonschema-linux-arm64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-windows-x86_64/bin/jsonschema.exe" \
  "$OUTPUT/github-releases/jsonschema-windows-x86_64.exe"

# (3) Run checks
npm ci
npm test
node npm/cli.js

# (4) Package
mkdir -p "$DIRECTORY"
npm pack --pack-destination "$DIRECTORY"
