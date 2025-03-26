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
PACKAGE_BASE_URL="https://github.com/sourcemeta/jsonschema/releases/download/v$VERSION"
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

# To boost NPM ranking
# See https://github.com/npms-io/npms-analyzer/blob/master/lib/analyze/evaluate/quality.js
install -m 0644 "$(pwd)/LICENSE" "$OUTPUT/npm/staging/LICENSE"
install -m 0644 "$(pwd)/.gitignore" "$OUTPUT/npm/staging/.gitignore"
echo "/build" > "$OUTPUT/npm/staging/.npmignore"

cat << EOF > "$OUTPUT/npm/staging/package.json"
{
  "name": "@sourcemeta/jsonschema",
  "version": "$VERSION",
  "description": "The CLI for working with JSON Schema. Covers formatting, linting, testing, and much more for both local development and CI/CD pipelines",
  "main": "cli.js",
  "bin": {
    "jsonschema": "cli.js"
  },
  "scripts": {
    "test": "eslint cli.js && node cli.js"
  },
  "license": "AGPL-3.0",
  "homepage": "https://github.com/sourcemeta/jsonschema",
  "author": {
    "email": "hello@sourcemeta.com",
    "name": "Sourcemeta",
    "url": "https://www.sourcemeta.com"
  },
  "os": [ "darwin", "linux", "win32" ],
  "cpu": [ "x64", "arm64" ],
  "engines": {
    "node": ">=16"
  },
  "funding": "https://github.com/sponsors/sourcemeta",
  "keywords": [
    "jsonschema", "json", "schema", "json-schema",
    "cli", "\$ref", "dereference", "reference", "resolve",
    "json-pointer", "validator", "validation", "bundle",
    "json-schema-validator", "json-schema-validation",
    "lint", "format", "draft"
  ],
  "bugs": {
    "url": "https://github.com/sourcemeta/jsonschema/issues"
  },
  "repository": {
    "type": "git",
    "url": "git+https://github.com/sourcemeta/jsonschema.git"
  },
  "publishConfig": {
    "provenance": true,
    "access": "public"
  },
  "devDependencies": {
    "@eslint/js": "^9.23.0",
    "eslint": "^9.23.0",
    "globals": "^16.0.0"
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
  console.error('Please open a GitHub issue at https://github.com/sourcemeta/jsonschema');
  process.exit(1);
}

if (PLATFORM === 'darwin') {
  child_process.spawnSync('/usr/bin/xattr', [ '-c', EXECUTABLE ], { stdio: 'inherit' });
}

const result = child_process.spawnSync(EXECUTABLE,
  process.argv.slice(2), { stdio: 'inherit' });
process.exit(result.status);
EOF

cat << 'EOF' > "$OUTPUT/npm/staging/eslint.config.mjs"
import { defineConfig } from "eslint/config";
import globals from "globals";
import js from "@eslint/js";

export default defineConfig([
  { files: ["**/*.{js,mjs,cjs}"] },
  { files: ["**/*.js"], languageOptions: { sourceType: "commonjs" } },
  { files: ["**/*.{js,mjs,cjs}"], languageOptions: { globals: globals.node } },
  { files: ["**/*.{js,mjs,cjs}"], plugins: { js }, extends: ["js/recommended"] },
]);
EOF

# (3) Run checks
cd "$OUTPUT/npm/staging"
npm install
npm test
cd -

# (4) Try packaging
cd "$OUTPUT/npm"
npm pack ./staging
cd -

# (5) Deploy to NPM
npm publish "$OUTPUT/npm/staging"
