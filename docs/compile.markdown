Compiling
=========

> [!WARNING]
> JSON Schema Draft 3 and older are not supported at this point in time.

```sh
jsonschema compile <schema.json|.yaml> [--http/-h] [--verbose/-v]
  [--resolve/-r <schemas-or-directories> ...] [--extension/-e <extension>]
  [--ignore/-i <schemas-or-directories>] [--fast/-f] [--default-dialect/-d <uri>]
  [--minify/-m] [--json/-j]
```

The `validate` command will first compile the schema into an optimised
low-level form (the compiled _template_) before evaluating using the
[Blaze](https://github.com/sourcemeta/blaze) high-performance JSON Schema
compiler. This command allows you to separately compile the schema and print
the low-level template to standard output.

**The low-level template is not stable across versions of this JSON Schema CLI.
While it might work, we don't necessarily support evaluating templates
generated with another version of this tool.**

You can pass the compiled template to the [`validate`](./validate.markdown)
command to avoid expensive schema compilation if you need to perform validation
multiple times with the same schema.

> [!WARNING]
> By default, schemas are compiled in exhaustive mode, which results in better
> error messages and annotations, at the expense of speed. The `--fast`/`-f`
> option makes the schema compiler optimise for speed, at the expense of error
> messages.

### Compile a standalone JSON Schema in exhaustive mode

```sh
jsonschema compile path/to/my/schema.json > template.json
```

### Compile a standalone JSON Schema in optimised mode

```sh
jsonschema compile path/to/my/schema.json --fast > template.json
```

### Compile a JSON Schema resolving one of its dependencies

```sh
jsonschema compile path/to/my/schema.json --resolve other.json > template.json
```
