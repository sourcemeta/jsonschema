import { spawnSync } from 'node:child_process';
import { existsSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const MODULE_DIR = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = resolve(MODULE_DIR, '../..');
const BINARY_NAME = 'sourcemeta_blaze_contrib_compile';

const CANDIDATES = [
  resolve(PROJECT_ROOT, 'build/contrib', BINARY_NAME),
  resolve(PROJECT_ROOT, 'build/bin/Release', BINARY_NAME + '.exe'),
  resolve(PROJECT_ROOT, 'build/bin/Debug', BINARY_NAME + '.exe')
];

const COMPILE_BIN = CANDIDATES.find(existsSync);
if (COMPILE_BIN) {
  process.stderr.write(`Found compile CLI: ${COMPILE_BIN}\n`);
} else {
  throw new Error('Could not find compile CLI. Searched:\n' + CANDIDATES.join('\n'));
}

export function compileSchema(schemaPath, options) {
  const args = [];

  if (options && options.mode === 'fast') {
    args.push('--fast');
  }

  if (options && options.defaultDialect) {
    args.push('--default-dialect', options.defaultDialect);
  }

  if (options && options.resolveDirectory) {
    args.push('--resolve-directory', options.resolveDirectory);
  }

  if (options && options.path) {
    args.push('--path', options.path);
  }

  args.push(schemaPath);

  const result = spawnSync(COMPILE_BIN, args, {
    encoding: 'utf-8',
    maxBuffer: 256 * 1024 * 1024
  });
  if (result.status !== 0) {
    throw new Error(result.stderr.trim());
  }

  return JSON.parse(result.stdout);
}
