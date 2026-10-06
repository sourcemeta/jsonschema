#!/bin/bash

set -o errexit
set -o nounset

TMP="$(mktemp -d)"
clean() { rm -rf "$TMP"; }
trap clean EXIT

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPLETION_SCRIPT="$(dirname "$SCRIPT_DIR")/completion/jsonschema.bash"

if ! [ -f "$COMPLETION_SCRIPT" ]
then
  echo "FAIL: Completion script not found at $COMPLETION_SCRIPT" 1>&2
  exit 1
fi

# shellcheck source=/dev/null
source "$COMPLETION_SCRIPT"

test_completion() {
  local input="$1"
  local expected="$2"
  local description="$3"

  read -r -a COMP_WORDS <<< "$input"

  if [[ "$input" == *" " ]]; then
    COMP_WORDS+=("")
  fi

  COMP_CWORD=$((${#COMP_WORDS[@]} - 1))
  COMP_LINE="$input"
  COMP_POINT=${#COMP_LINE}
  COMPREPLY=()

  _jsonschema

  local found=0
  if [ "${#COMPREPLY[@]}" -gt 0 ]; then
    for completion in "${COMPREPLY[@]}"
    do
      if [ "$completion" = "$expected" ]
      then
        found=1
        break
      fi
    done
  fi

  if [ $found -eq 0 ]
  then
    echo "FAIL: $description" 1>&2
    echo "  Input: $input" 1>&2
    echo "  Expected: $expected" 1>&2
    if [ "${#COMPREPLY[@]}" -gt 0 ]; then
      echo "  Got: ${COMPREPLY[*]}" 1>&2
    else
      echo "  Got: (no completions)" 1>&2
    fi
    return 1
  fi
}

test_no_completion() {
  local input="$1"
  local description="$2"

  read -r -a COMP_WORDS <<< "$input"

  if [[ "$input" == *" " ]]; then
    COMP_WORDS+=("")
  fi

  COMP_CWORD=$((${#COMP_WORDS[@]} - 1))
  COMP_LINE="$input"
  COMP_POINT=${#COMP_LINE}
  COMPREPLY=()

  _jsonschema

  if [ "${#COMPREPLY[@]}" -gt 0 ]
  then
    echo "FAIL: $description" 1>&2
    echo "  Input: $input" 1>&2
    echo "  Expected: (no completions)" 1>&2
    echo "  Got: ${COMPREPLY[*]}" 1>&2
    return 1
  fi
}

test_completion_excludes() {
  local input="$1"
  local unexpected="$2"
  local description="$3"

  read -r -a COMP_WORDS <<< "$input"

  if [[ "$input" == *" " ]]; then
    COMP_WORDS+=("")
  fi

  COMP_CWORD=$((${#COMP_WORDS[@]} - 1))
  COMP_LINE="$input"
  COMP_POINT=${#COMP_LINE}
  COMPREPLY=()

  _jsonschema

  if [ "${#COMPREPLY[@]}" -gt 0 ]; then
    for completion in "${COMPREPLY[@]}"
    do
      if [ "$completion" = "$unexpected" ]
      then
        echo "FAIL: $description" 1>&2
        echo "  Input: $input" 1>&2
        echo "  Did not expect: $unexpected" 1>&2
        echo "  Got: ${COMPREPLY[*]}" 1>&2
        return 1
      fi
    done
  fi
}

test_completion "jsonschema " "validate" "Command completion includes validate"
test_completion "jsonschema " "metaschema" "Command completion includes metaschema"
test_completion "jsonschema " "compile" "Command completion includes compile"
test_completion "jsonschema " "test" "Command completion includes test"
test_completion "jsonschema " "fmt" "Command completion includes fmt"
test_completion "jsonschema " "lint" "Command completion includes lint"
test_completion "jsonschema " "bundle" "Command completion includes bundle"
test_completion "jsonschema " "inspect" "Command completion includes inspect"
test_completion "jsonschema " "encode" "Command completion includes encode"
test_completion "jsonschema " "decode" "Command completion includes decode"
test_completion "jsonschema " "codegen" "Command completion includes codegen"
test_completion "jsonschema " "install" "Command completion includes install"
test_completion "jsonschema " "upgrade" "Command completion includes upgrade"
test_completion "jsonschema " "rdf" "Command completion includes rdf"
test_completion "jsonschema " "llm" "Command completion includes llm"
test_completion "jsonschema " "version" "Command completion includes version"
test_completion "jsonschema " "help" "Command completion includes help"

test_completion "jsonschema validate --" "--verbose" "Validate includes global option --verbose"
test_completion "jsonschema validate --" "--benchmark" "Validate includes --benchmark"
test_completion "jsonschema validate --" "--trace" "Validate includes --trace"
test_completion "jsonschema validate --" "--fast" "Validate includes --fast"
test_completion "jsonschema validate --" "--valid" "Validate includes --valid"
test_completion "jsonschema validate --" "--invalid" "Validate includes --invalid"

test_completion "jsonschema lint --" "--fix" "Lint includes --fix"
test_completion "jsonschema lint --" "--list" "Lint includes --list"
test_completion "jsonschema lint --" "--rule" "Lint includes --rule"
test_completion "jsonschema lint --" "--top-level-rule" "Lint includes --top-level-rule"

test_completion "jsonschema bundle --" "--without-id" "Bundle includes --without-id"

test_completion "jsonschema compile --" "--minify" "Compile includes --minify"

test_completion "jsonschema fmt --" "--check" "Fmt includes --check"
test_completion "jsonschema fmt --" "--keep-ordering" "Fmt includes --keep-ordering"

test_completion "jsonschema codegen --" "--name" "Codegen includes --name"
test_completion "jsonschema codegen --" "--target" "Codegen includes --target"
test_completion "jsonschema codegen --" "--verbose" "Codegen includes global option --verbose"

test_completion "jsonschema install --" "--force" "Install includes --force"
test_completion "jsonschema install --" "--frozen" "Install includes --frozen"
test_completion "jsonschema install --" "--verbose" "Install includes global option --verbose"
test_completion "jsonschema install --" "--debug" "Install includes global option --debug"

test_completion "jsonschema rdf --" "--flatten" "Rdf includes --flatten"
test_completion "jsonschema rdf --" "--compact" "Rdf includes --compact"
test_completion "jsonschema rdf --" "--fast" "Rdf includes --fast"
test_completion "jsonschema rdf --" "--format-assertion" "Rdf includes --format-assertion"
test_completion "jsonschema rdf --" "--verbose" "Rdf includes global option --verbose"

test_completion "jsonschema validate --" "--header" "Validate includes global option --header"
test_no_completion "jsonschema validate --header " "After --header no completion is offered"
test_no_completion "jsonschema validate -H " "After -H no completion is offered"

test_completion "jsonschema help --" "--color" "Help includes global option --color"
test_completion "jsonschema help --color " "auto" "After --color auto is offered"
test_completion "jsonschema help --color " "always" "After --color always is offered"
test_completion "jsonschema help --color " "never" "After --color never is offered"
test_completion "jsonschema validate --" "--color" "Validate includes global option --color"

test_completion "jsonschema upgrade --to " "2020-12" "After --to 2020-12 is offered"
test_completion "jsonschema llm --" "--ask" "LLM includes --ask"
test_completion "jsonschema llm --" "--url" "LLM includes --url"
test_completion "jsonschema llm --" "--model" "LLM includes --model"
test_completion "jsonschema llm --" "--param" "LLM includes --param"
test_completion "jsonschema llm --" "--dry-run" "LLM includes --dry-run"
test_completion "jsonschema llm --" "--verbose" "LLM includes global option --verbose"
test_completion "jsonschema llm --upgrade " "2020-12" "After --upgrade 2020-12 is offered"
test_completion "jsonschema llm --upgrade " "draft7" "After --upgrade draft7 is offered"
test_completion "jsonschema llm -U " "draft4" "After -U draft4 is offered"
test_completion "jsonschema llm -" "-a" "LLM offers -a for --ask"
test_completion "jsonschema llm -" "-m" "LLM offers -m for --model"
test_completion "jsonschema llm -" "-p" "LLM offers -p for --param"
test_completion "jsonschema upgrade --to " "openapi3.1" "After --to openapi3.1 is offered"
test_completion "jsonschema upgrade --to " "openapi3.2" "After --to openapi3.2 is offered"
test_completion "jsonschema upgrade -t " "openapi3.1" "After -t openapi3.1 is offered"
test_completion "jsonschema upgrade -t " "openapi3.2" "After -t openapi3.2 is offered"

cd "$TMP"
touch a.json b.yaml c.yml d.jsonl e.binpack f.txt

test_completion "jsonschema llm " "a.json" "A schema may be a .json file"
test_completion "jsonschema llm " "b.yaml" "A schema may be a .yaml file"
test_completion "jsonschema llm " "c.yml" "A schema may be a .yml file"
test_completion_excludes "jsonschema llm " "f.txt" "A schema is not any file"
test_completion_excludes "jsonschema llm " "d.jsonl" "A schema is not a dataset"

test_completion "jsonschema validate " "a.json" "An instance may be a .json file"
test_completion "jsonschema validate " "c.yml" "An instance may be a .yml file"
test_completion "jsonschema validate " "d.jsonl" "An instance may be a .jsonl dataset"
test_completion_excludes "jsonschema validate " "f.txt" "An instance is not any file"

test_completion "jsonschema validate -m " "a.json" "A template is a .json file"
test_completion_excludes "jsonschema validate -m " "b.yaml" "A template is not YAML"

test_completion "jsonschema lint -a " "b.yaml" "A lint rule may be a .yaml file"

test_completion "jsonschema encode " "a.json" "Encoding takes a .json file"
test_completion "jsonschema encode " "d.jsonl" "Encoding takes a .jsonl dataset"
test_completion_excludes "jsonschema encode " "b.yaml" "Encoding does not take YAML"

test_completion "jsonschema decode " "e.binpack" "Decoding takes a .binpack file"
test_completion_excludes "jsonschema decode " "a.json" "Decoding does not take JSON"

echo "PASS" 1>&2
