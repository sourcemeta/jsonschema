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

Blaze evaluates pre-compiled schema templates. Compile your JSON
Schema using the
[jsonschema](https://github.com/sourcemeta/jsonschema) CLI
([compile docs](https://github.com/sourcemeta/jsonschema/blob/main/docs/compile.markdown)):

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
