import { describe, it } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { join, resolve, dirname, basename } from 'node:path';
import { fileURLToPath } from 'node:url';
import { compileSchema } from './compile.mjs';
import { Blaze } from './index.mjs';

const MODULE_DIR = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = resolve(MODULE_DIR, '../..');
const SUITE_PATH = join(PROJECT_ROOT, 'vendor/jsonschema-test-suite');
const REMOTES_PATH = join(SUITE_PATH, 'remotes');
const RESOLVE_DIRECTORY = `http://localhost:1234,${REMOTES_PATH}`;

const BLACKLISTS = {
  'draft2020-12': new Set(),
  'draft2020-12/optional': new Set(['format-assertion', 'refOfUnknownKeyword']),
  'draft2020-12/optional/format': new Set([
    'date-time', 'date', 'duration', 'email', 'hostname', 'idn-email',
    'idn-hostname', 'ipv4', 'ipv6', 'iri-reference', 'iri', 'json-pointer',
    'regex', 'relative-json-pointer', 'time', 'uri-reference', 'uri-template',
    'uri', 'uuid', 'ecmascript-regex'
  ]),
  'draft2019-09': new Set(),
  'draft2019-09/optional': new Set(['refOfUnknownKeyword']),
  'draft2019-09/optional/format': new Set([
    'date-time', 'date', 'duration', 'email', 'hostname', 'idn-email',
    'idn-hostname', 'ipv4', 'ipv6', 'iri-reference', 'iri', 'json-pointer',
    'regex', 'relative-json-pointer', 'time', 'uri-reference', 'uri-template',
    'uri', 'uuid'
  ]),
  'draft7': new Set(),
  'draft7/optional': new Set(['content']),
  'draft7/optional/format': new Set([
    'date-time', 'date', 'email', 'hostname', 'idn-email', 'idn-hostname',
    'ipv4', 'ipv6', 'iri-reference', 'iri', 'json-pointer', 'regex',
    'relative-json-pointer', 'time', 'unknown', 'uri-reference', 'uri-template',
    'uri'
  ]),
  'draft6': new Set(),
  'draft6/optional': new Set(),
  'draft6/optional/format': new Set([
    'date-time', 'email', 'hostname', 'ipv4', 'ipv6', 'json-pointer',
    'unknown', 'uri-reference', 'uri-template', 'uri'
  ]),
  'draft4': new Set(),
  'draft4/optional': new Set(['zeroTerminatedFloats']),
  'draft4/optional/format': new Set([
    'date-time', 'email', 'hostname', 'ipv4', 'ipv6', 'uri'
  ])
};

const DIALECTS = {
  'draft2020-12': 'https://json-schema.org/draft/2020-12/schema',
  'draft2019-09': 'https://json-schema.org/draft/2019-09/schema',
  'draft7': 'http://json-schema.org/draft-07/schema#',
  'draft6': 'http://json-schema.org/draft-06/schema#',
  'draft4': 'http://json-schema.org/draft-04/schema#'
};

function registerTests(subdirectory, defaultDialect, blacklist) {
  const testsDir = join(SUITE_PATH, 'tests', subdirectory);
  if (!existsSync(testsDir)) return;

  const files = readdirSync(testsDir)
    .filter(file => file.endsWith('.json'))
    .sort();

  describe(subdirectory, () => {
    for (const file of files) {
      const name = basename(file, '.json');
      if (blacklist.has(name)) continue;

      const filePath = join(testsDir, file);
      console.error(`Loading ${subdirectory}/${name}`);
      const suite = JSON.parse(readFileSync(filePath, 'utf-8'));

      for (let groupIndex = 0; groupIndex < suite.length; groupIndex++) {
        const testGroup = suite[groupIndex];

        for (const mode of ['fast', 'exhaustive']) {
          let evaluator;
          try {
            evaluator = new Blaze(compileSchema(filePath, {
              mode,
              defaultDialect,
              resolveDirectory: RESOLVE_DIRECTORY,
              path: `/${groupIndex}/schema`
            }));
          } catch (error) {
            it(`[${mode}] ${testGroup.description} (compile error: ${error.message})`, () => {
              assert.fail(`Compilation failed: ${error.message}`);
            });
            continue;
          }

          for (const testCase of testGroup.tests) {
            const testName = `[${mode}] ${testGroup.description} - ${testCase.description}`;
            it(testName, () => {
              const result = evaluator.validate(testCase.data);
              assert.equal(result, testCase.valid,
                `Expected ${testCase.valid} but got ${result}`);
            });
          }
        }
      }
    }
  });
}

for (const [subdirectory, blacklist] of Object.entries(BLACKLISTS)) {
  const draftKey = subdirectory.split('/')[0];
  const defaultDialect = DIALECTS[draftKey];
  registerTests(subdirectory, defaultDialect, blacklist);
}
