# @sourcemeta/blaze

[![NPM Version](https://img.shields.io/npm/v/@sourcemeta/blaze)](https://www.npmjs.com/package/@sourcemeta/blaze)
[![NPM Downloads](https://img.shields.io/npm/dm/%40sourcemeta%2Fblaze)](https://www.npmjs.com/package/@sourcemeta/blaze)
[![GitHub contributors](https://img.shields.io/github/contributors/sourcemeta/blaze.svg)](https://github.com/sourcemeta/blaze/graphs/contributors/)

A pure JavaScript port of the evaluator from
[Blaze](https://github.com/sourcemeta/blaze), a high-performance
C++ JSON Schema validator. Zero dependencies. Supports Draft 4,
Draft 6, Draft 7, 2019-09, and 2020-12 with schema-specific code
generation for near-native speed.

## Install

```sh
npm install --save @sourcemeta/blaze
```

## Usage

Blaze evaluates pre-compiled schema templates. Compile your JSON Schema using
the [JSON Schema CLI](https://github.com/sourcemeta/jsonschema) (see the
[`compile`](https://github.com/sourcemeta/jsonschema/blob/main/docs/compile.markdown)
command):

```sh
npm install --global @sourcemeta/jsonschema

cat > schema.json <<'EOF'
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "age": { "type": "integer" }
  },
  "required": [ "name" ]
}
EOF

jsonschema compile schema.json --fast > template.json
```

Then validate instances:

```javascript
import { readFileSync } from "node:fs";
import { Blaze } from "@sourcemeta/blaze";

const template =
  JSON.parse(readFileSync("template.json", "utf-8"));
const evaluator = new Blaze(template);

// true or false
console.log(evaluator.validate({ name: "John", age: 30 }));
```

With an evaluation callback for tracing:

```javascript
const instance = { name: "John", age: 30 };
const result = evaluator.validate(instance,
  (type, valid, instruction,
   evaluatePath, instanceLocation, annotation) => {
    console.log(type, evaluatePath,
      instanceLocation, valid);
  });
console.log(result); // true or false
```

### Parsing large integers

JavaScript's
[`JSON.parse`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/JSON/parse)
silently truncates integers that exceed
[`Number.MAX_SAFE_INTEGER`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Number/MAX_SAFE_INTEGER)
to IEEE 754 double-precision floats. If your schemas or instances contain large
integers, pass `Blaze.reviver` to `JSON.parse` to preserve them as
[`BigInt`](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/BigInt)
values. This relies on the
[`JSON.parse` source text
access](https://github.com/tc39/proposal-json-parse-with-source) proposal
(Stage 4, shipped in all major engines):

```javascript
const template =
  JSON.parse(readFileSync("template.json", "utf-8"), Blaze.reviver);
const evaluator = new Blaze(template);

const instance =
  JSON.parse(readFileSync("instance.json", "utf-8"), Blaze.reviver);
console.log(evaluator.validate(instance));
```

The evaluator handles `BigInt` values natively in all numeric instructions.
Without a reviver, large integers are silently truncated and validation may
produce incorrect results for affected values.

## Why compile separately?

Unlike validators that compile and evaluate in a single step, Blaze separates
the two. You compile your schemas ahead of time using the [JSON Schema
CLI](https://github.com/sourcemeta/jsonschema) (`jsonschema compile
schema.json`) on CI or at build time.

The compilation step analyses the schema, resolves all references, and produces
a set of optimised low-level instructions serialised as JSON that every
evaluator can rely on. This evaluator package then executes those instructions
against your data, with no knowledge of JSON Schema itself.

- **Consistent validation across languages.** The same compiled schema produces
  identical results whether evaluated in JavaScript, Python, Java, or C++ (we
  are working hard increase the number of ports) so you never have to rely on
  multiple implementations with different interpretations of the standard

- **No schema processing at startup.** The evaluator loads pre-compiled
  instructions. There is no reference resolution, or vocabulary processing at
  runtime

- **Schema governance.** Compiled schemas can be managed, versioned, and
  distributed centrally via [Sourcemeta One](https://one.sourcemeta.com),
  ensuring all consumers validate against the same compiled artifact

## Limitations

**No arbitrary-precision real number support.** Large integers can be preserved
as `BigInt` using a reviver (see above), but JavaScript has no equivalent type
for arbitrary-precision real numbers. `JSON.parse` truncates values like
`0.1000000000000000000000000000000001` to IEEE 754 doubles, and there is no
reviver-based workaround:

```
$ node --eval "console.log(JSON.parse('0.1000000000000000000000000000000001'))"
0.1
```

Numeric keywords like `multipleOf` that depend on exact decimal arithmetic may
produce incorrect results for real values that lose precision during parsing.
The TC39 [Decimal proposal](https://github.com/tc39/proposal-decimal) (Stage 2)
aims to address this in the future.
