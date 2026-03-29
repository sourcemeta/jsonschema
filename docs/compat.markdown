Schema Compatibility
====================

```sh
jsonschema compat <base-schema.json|.yaml> <candidate-schema.json|.yaml>
  [--http/-h] [--verbose/-v] [--debug/-g]
  [--json/-j] [--mode/-m <backward|forward|full>]
  [--format/-f <text|json>] [--fail-on/-x <none|warning|breaking>]
```

The `compat` command is a prototype for a future schema compatibility checker
workflow in the JSON Schema CLI. Its current job is to establish the command
surface, validate inputs, and shape the output contract that a semantic
comparison engine can later implement.

The command accepts two schemas:

- A base schema representing the current version
- A candidate schema representing the proposed next version

The comparison mode can be selected with `--mode`:

- `backward` checks whether existing consumers can continue to work with the
  candidate schema
- `forward` checks whether new consumers can still interpret data described by
  the base schema
- `full` reserves a mode for future bidirectional analysis

The `--format` option selects either human-readable text or machine-readable
JSON. Passing the global `--json` option is equivalent to `--format json`.

> [!NOTE]
> This prototype intentionally does not emit semantic compatibility findings
> yet. It is a qualification-task oriented scaffold for integrating the
> open-sourced compatibility engine into the CLI.

Examples
--------

### Compare two schemas in backward mode

```sh
jsonschema compat path/to/base.schema.json path/to/next.schema.json
```

### Compare two schemas and emit JSON

```sh
jsonschema compat path/to/base.schema.json path/to/next.schema.json --json
```

### Reserve a stricter CI threshold

```sh
jsonschema compat path/to/base.schema.json path/to/next.schema.json \
  --mode full --fail-on warning
```
