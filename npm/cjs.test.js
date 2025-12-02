const test = require('node:test');
const assert = require('node:assert');
const { spawn } = require('./main.js');
const packageJson = require('../package.json');

// This file is just to do a basic pass and ensure that CommonJS
// imports work. Please put actual tests in the other ESM file.

test('commonjs: spawn returns version with --version flag', async () => {
  const result = await spawn(['--version']);
  assert.strictEqual(result.code, 0);
  assert.strictEqual(result.stdout.trim(), packageJson.version);
});
