Decode
======

```sh
jsonschema decode <output.binpack> <output.json|.jsonl>
  [--default-dialect/-d <uri>]
```

This command decodes a JSON document using [JSON
BinPack](https://jsonbinpack.sourcemeta.com) schema-less mode. **Note this
command is considered experimental and might not decode binary files produced
by other versions of this CLI**.

Examples
--------

For example, consider the following encoded file:

```
$ xxd output.binpack
00000000: 1308 7665 7273 696f 6e37 02              ..version7.
```

Decoding this file using JSON BinPack will result in the following document:

```json
{
  "version": 2.0
}
```

### Decode a binary file into a JSON document

```sh
jsonschema decode path/to/output.binpack path/to/my/output.json
```

### Decode a binary file into a JSONL dataset

```sh
jsonschema decode path/to/output.binpack path/to/my/dataset.jsonl
```
