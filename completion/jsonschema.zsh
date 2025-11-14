#compdef jsonschema
# shellcheck shell=bash disable=SC2034,SC2154,SC1087,SC2125,SC2068

_jsonschema() {
  local context state state_descr line
  typeset -A opt_args

  local -a commands
  commands=(
    'validate:Validate instances against a schema'
    'metaschema:Validate schemas against their metaschemas'
    'compile:Compile a schema into an optimised representation'
    'test:Run unit tests against a schema'
    'fmt:Format schemas in-place or check formatting'
    'lint:Lint schemas and optionally fix issues'
    'bundle:Inline remote references in a schema'
    'inspect:Display schema locations and references'
    'encode:Encode JSON using JSON BinPack'
    'decode:Decode JSON using JSON BinPack'
    'version:Print version information'
    'help:Print help information'
  )

  local -a global_options
  global_options=(
    '(--verbose -v)'{--verbose,-v}'[Enable verbose output]'
    '(--resolve -r)'{--resolve,-r}'[Import schemas into resolution context]:schema file:_files -g "*.json *.yaml *.yml"'
    '(--default-dialect -d)'{--default-dialect,-d}'[Specify default dialect URI]:dialect URI:_jsonschema_dialects'
    '(--json -j)'{--json,-j}'[Prefer JSON output if supported]'
    '(--http -h)'{--http,-h}'[Enable HTTP resolution]'
  )

  _arguments -C \
    '1: :->command' \
    '*:: :->option-or-argument' \
    && return 0

  case $state in
    command)
      _describe -t commands 'jsonschema command' commands
      ;;
    option-or-argument)
      local command=$line[1]
      case $command in
        validate)
          _arguments \
            ${global_options[@]} \
            '(--benchmark -b)'{--benchmark,-b}'[Enable benchmarking mode]' \
            '(--loop -l)'{--loop,-l}'[Number of loop iterations]:iterations:' \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--trace -t)'{--trace,-t}'[Enable trace output]' \
            '(--fast -f)'{--fast,-f}'[Optimise for speed]' \
            '(--template -m)'{--template,-m}'[Use pre-compiled schema template]:template file:_files -g "*.json"' \
            '1:schema file:_files -g "*.json *.yaml *.yml"' \
            '*:instance file:_files -g "*.json *.yaml *.yml *.jsonl"'
          ;;
        metaschema)
          _arguments \
            ${global_options[@]} \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--trace -t)'{--trace,-t}'[Enable trace output]' \
            '*:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        compile)
          _arguments \
            ${global_options[@]} \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--fast -f)'{--fast,-f}'[Optimise for speed]' \
            '(--minify -m)'{--minify,-m}'[Minify output]' \
            '1:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        test)
          _arguments \
            ${global_options[@]} \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '*:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        fmt)
          _arguments \
            ${global_options[@]} \
            '(--check -c)'{--check,-c}'[Check formatting without modifying]' \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--keep-ordering -k)'{--keep-ordering,-k}'[Keep original key ordering]' \
            '(--indentation -n)'{--indentation,-n}'[Specify indentation spaces]:spaces:(2 4 8)' \
            '*:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        lint)
          _arguments \
            ${global_options[@]} \
            '(--fix -f)'{--fix,-f}'[Fix issues automatically]' \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--exclude -x)'{--exclude,-x}'[Exclude specific rule]:rule name:' \
            '(--only -o)'{--only,-o}'[Only run specific rule]:rule name:' \
            '(--list -l)'{--list,-l}'[List all enabled rules]' \
            '(--strict -s)'{--strict,-s}'[Enable strict mode]' \
            '(--indentation -n)'{--indentation,-n}'[Specify indentation spaces]:spaces:(2 4 8)' \
            '*:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        bundle)
          _arguments \
            ${global_options[@]} \
            '(--extension -e)'{--extension,-e}'[Specify file extension]:extension:_jsonschema_extensions' \
            '(--ignore -i)'{--ignore,-i}'[Ignore schemas or directories]:path:_files' \
            '(--without-id -w)'{--without-id,-w}'[Bundle without ID]' \
            '1:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        inspect)
          _arguments \
            ${global_options[@]} \
            '1:schema file:_files -g "*.json *.yaml *.yml"'
          ;;
        encode)
          _arguments \
            ${global_options[@]} \
            '1:input file:_files -g "*.json *.jsonl"' \
            '2:output file:_files'
          ;;
        decode)
          _arguments \
            ${global_options[@]} \
            '1:input file:_files -g "*.binpack"' \
            '2:output file:_files -g "*.json *.jsonl"'
          ;;
        version|help)
          ;;
      esac
      ;;
  esac
}

_jsonschema_extensions() {
  local -a extensions
  extensions=('.json' '.yaml' '.yml')
  _describe -t extensions 'file extension' extensions
}

_jsonschema_dialects() {
  local -a dialects
  dialects=(
    'https://json-schema.org/draft/2020-12/schema:Draft 2020-12'
    'https://json-schema.org/draft/2019-09/schema:Draft 2019-09'
    'https://json-schema.org/draft-07/schema:Draft 07'
    'https://json-schema.org/draft-06/schema:Draft 06'
    'https://json-schema.org/draft-04/schema:Draft 04'
  )
  _describe -t dialects 'JSON Schema dialect' dialects
}

_jsonschema "$@"
