import test from 'node:test';
import assert from 'node:assert';
import { mkdtemp, writeFile, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { spawn } from './main.js';
import packageJson from '../package.json' with { type: 'json' };

test('spawn returns version with --version flag', async () => {
  const result = await spawn(['--version']);
  assert.strictEqual(result.code, 0);
  assert.strictEqual(result.stdout.trim(), packageJson.version);
});

test('spawn returns help with --help flag', async () => {
  const result = await spawn(['--help']);
  assert.strictEqual(result.code, 0);
  assert.match(result.stdout, /Usage:/);
  assert.match(result.stdout, /Commands:/);
});

test('spawn returns non-zero exit code for invalid command', async () => {
  const result = await spawn(['invalid-command']);
  assert.strictEqual(result.code, 1);
});

test('spawn captures stderr on error', async () => {
  const result = await spawn(['validate']);
  assert.strictEqual(result.code, 1);
  assert.ok(result.stderr.length > 0);
});

test('spawn with json option appends --json and parses stdout', async () => {
  const tempDir = await mkdtemp(join(tmpdir(), 'jsonschema-test-'));
  const schemaPath = join(tempDir, 'schema.json');
  await writeFile(schemaPath, JSON.stringify({
    "$schema": "https://json-schema.org/draft/2020-12/schema",
    "type": "string"
  }));

  try {
    const result = await spawn(['inspect', schemaPath], { json: true });
    assert.strictEqual(result.code, 0);
    assert.strictEqual(typeof result.stdout, 'object');
    assert.ok(result.stdout.locations);
    assert.ok(result.stdout.references);
  } finally {
    await rm(tempDir, { recursive: true });
  }
});

test('spawn with json option on error returns parsed JSON error', async () => {
  const result = await spawn(['inspect', '/nonexistent/path.json'], { json: true });
  assert.strictEqual(result.code, 1);
  assert.deepStrictEqual(result.stdout, {
    error: 'No such file or directory',
    filePath: '/nonexistent/path.json'
  });
});
