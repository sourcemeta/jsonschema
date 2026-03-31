import { compileSchema } from './compile.mjs';
import { Blaze } from './index.mjs';
import { readFileSync, readdirSync, existsSync } from 'node:fs';
import { join, resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const MODULE_DIR = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = resolve(MODULE_DIR, '../..');
const BENCHMARK_DIR = join(PROJECT_ROOT, 'benchmark/e2e');

const MINIMUM_DURATION_NS = 1_000_000_000;
const WARMUP_ITERATIONS = 3;

function measure(callback) {
  for (let index = 0; index < WARMUP_ITERATIONS; index++) {
    callback();
  }

  let iterations = 0;
  const start = process.hrtime.bigint();
  while (true) {
    callback();
    iterations++;
    const elapsed = process.hrtime.bigint() - start;
    if (elapsed >= MINIMUM_DURATION_NS) {
      return Number(elapsed) / iterations;
    }
  }
}

function loadCase(name) {
  const directory = join(BENCHMARK_DIR, name);
  const schemaPath = join(directory, 'schema.json');
  const instancesPath = join(directory, 'instances.jsonl');
  if (!existsSync(schemaPath) || !existsSync(instancesPath)) return null;

  const evaluator = new Blaze(compileSchema(schemaPath, { mode: 'fast' }));
  const lines = readFileSync(instancesPath, 'utf-8').trim().split('\n');
  const instances = lines.map(line => JSON.parse(line));
  return { evaluator, instances };
}

const dirs = readdirSync(BENCHMARK_DIR)
  .filter(entry => !entry.endsWith('.cc'))
  .sort();

const cases = {};
for (const directory of dirs) {
  try {
    const loaded = loadCase(directory);
    if (loaded) {
      cases[directory] = loaded;
    }
  } catch (error) {
    process.stderr.write(`Skipping ${directory}: ${error.message.split('\n')[0]}\n`);
  }
}

for (const [name, { evaluator, instances }] of Object.entries(cases)) {
  for (const instance of instances) {
    if (!evaluator.validate(instance)) {
      throw new Error(`${name}: validation returned false`);
    }
  }
}

function formatNumber(value) {
  return Math.round(value).toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
}

const names = Object.keys(cases);
const longest = Math.max(...names.map(name => `E2E_Evaluator_${name}`.length));
const results = [];

process.stderr.write('\n');
for (const [name, { evaluator, instances }] of Object.entries(cases)) {
  const label = `E2E_Evaluator_${name}`;
  const nanoseconds = Math.round(measure(() => {
    for (const instance of instances) {
      evaluator.validate(instance);
    }
  }));

  const padding = ' '.repeat(longest - label.length + 4);
  process.stderr.write(`${label}${padding}${formatNumber(nanoseconds)} ns/iter\n`);
  results.push({ name: label, unit: 'ns', value: nanoseconds });
}

process.stdout.write(JSON.stringify(results, null, 2) + '\n');
