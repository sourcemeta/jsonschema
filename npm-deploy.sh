#!/bin/sh

set -o errexit
set -o nounset

if [ $# -lt 1 ]
then
  echo "Usage: $0 <version>" 1>&2
  exit 1
fi

OUTPUT="$(pwd)/build"
VERSION="$(echo "$1" | tr -d 'v')"

# (1) Download artifacts
mkdir -p "$OUTPUT/npm/artifacts"
echo "Preparing $VERSION" 1>&2
PACKAGE_BASE_URL="https://github.com/intelligence-ai/jsonschema/releases/download/v$VERSION"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/darwin-arm64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-arm64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/darwin-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-darwin-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/linux-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-linux-x86_64.zip"
curl --retry 5 --location --output "$OUTPUT/npm/artifacts/windows-x86_64.zip" \
  "$PACKAGE_BASE_URL/jsonschema-$VERSION-windows-x86_64.zip"
unzip -o "$OUTPUT/npm/artifacts/darwin-arm64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/darwin-x86_64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/linux-x86_64.zip" -d "$OUTPUT/npm/artifacts"
unzip -o "$OUTPUT/npm/artifacts/windows-x86_64.zip" -d "$OUTPUT/npm/artifacts"
ls -l "$OUTPUT/npm/artifacts"

# (2) Stage package contents
rm -rf "$OUTPUT/npm/staging"
mkdir -p "$OUTPUT/npm/staging"

install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-darwin-arm64/bin/jsonschema" \
  "$OUTPUT/npm/staging/jsonschema-darwin-arm64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-darwin-x86_64/bin/jsonschema" \
  "$OUTPUT/npm/staging/jsonschema-darwin-x86_64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-linux-x86_64/bin/jsonschema" \
  "$OUTPUT/npm/staging/jsonschema-linux-x86_64"
install -m 0755 "$OUTPUT/npm/artifacts/jsonschema-$VERSION-windows-x86_64/bin/jsonschema.exe" \
  "$OUTPUT/npm/staging/jsonschema-windows-x86_64.exe"
install -m 0644 "$(pwd)/README.markdown" "$OUTPUT/npm/staging/README.md"

cat << EOF > "$OUTPUT/npm/staging/package.json"
{
  "name": "@intelligence-ai/jsonschema",
  "version": "$VERSION",
  "description": "The CLI for working with JSON Schema. Covers formatting, linting, testing, and much more for both local development and CI/CD pipelines",
  "main": "cli.js",
  "bin": {
    "jsonschema": "cli.js"
  },
  "license": "AGPL-3.0",
  "homepage": "https://github.com/intelligence-ai/jsonschema",
  "author": "Juan Cruz Viotti <juan@intelligence.ai>",
  "keywords": [
    "jsonschema", "json", "schema", "json-schema",
    "cli", "\$ref", "dereference", "reference", "resolve",
    "json-pointer", "validator", "bundle",
    "lint", "format"
  ],
  "bugs": {
    "url": "https://github.com/intelligence-ai/jsonschema/issues"
  },
  "repository": {
    "type": "git",
    "url": "git+https://github.com/intelligence-ai/jsonschema.git"
  },
  "publishConfig": {
    "access": "public"
  }
}
EOF

cat << 'EOF' > "$OUTPUT/npm/staging/cli.js"
#!/usr/bin/env node
const os = require('os');
const path = require('path');
const fs = require('fs');
const child_process = require('child_process');

const PLATFORM = os.platform() === 'win32' ? 'windows' : os.platform();
const ARCH = os.arch() === 'x64' ? 'x86_64' : os.arch();
const EXECUTABLE = PLATFORM === 'windows'
  ? path.join(__dirname, `jsonschema-${PLATFORM}-${ARCH}.exe`)
  : path.join(__dirname, `jsonschema-${PLATFORM}-${ARCH}`);

if (!fs.existsSync(EXECUTABLE)) {
  console.error(`The JSON Schema CLI NPM package does not support ${os.platform()} for ${ARCH} yet`);
  console.error('Please open a GitHub issue at https://github.com/Intelligence-AI/jsonschema');
  process.exit(1);
}

if (PLATFORM === 'darwin') {
  child_process.spawnSync('/usr/bin/xattr', [ '-c', EXECUTABLE ], { stdio: 'inherit' });
}

const result = child_process.spawnSync(EXECUTABLE,
  process.argv.slice(2), { stdio: 'inherit' });
process.exit(result.status);
EOF

# (3) Try packaging
cd "$OUTPUT/npm"
npm pack ./staging
cd -

# (4) Deploy to NPM
npm publish "$OUTPUT/npm/staging"
