#!/usr/bin/env node
const os = require('os');
const path = require('path');
const fs = require('fs');
const child_process = require('child_process');

const PLATFORM = os.platform() === 'win32' ? 'windows' : os.platform();
const ARCH = os.arch() === 'x64' ? 'x86_64' : os.arch();
const EXECUTABLE = PLATFORM === 'windows'
  ? path.join(__dirname, 'build', 'github-releases', `jsonschema-${PLATFORM}-${ARCH}.exe`)
  : path.join(__dirname, 'build', 'github-releases', `jsonschema-${PLATFORM}-${ARCH}`);

if (!fs.existsSync(EXECUTABLE)) {
  console.error(`The JSON Schema CLI NPM package does not support ${os.platform()} for ${ARCH} yet`);
  console.error('Please open a GitHub issue at https://github.com/sourcemeta/jsonschema');
  process.exit(1);
}

if (PLATFORM === 'darwin') {
  child_process.spawnSync('/usr/bin/xattr', [ '-c', EXECUTABLE ], { stdio: 'inherit' });
}

const result = child_process.spawnSync(EXECUTABLE, process.argv.slice(2), {
  stdio: 'inherit',
  // Do not open a command prompt on spawning
  windowsHide: true
});

process.exit(result.status);
