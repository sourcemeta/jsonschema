# `compgen -X` honours only the last pattern it is given, so each extension is
# matched in a pass of its own and the results are joined
_jsonschema_files() {
  local current="$1"
  shift
  local extension
  for extension in "$@"
  do
    compgen -f -X "!*${extension}" -- "${current}"
  done
}

# shellcheck disable=SC2207
_jsonschema() {
  local current previous commands global_options
  COMPREPLY=()
  current="${COMP_WORDS[COMP_CWORD]}"

  if [ "${COMP_CWORD}" -gt 0 ]
  then
    previous="${COMP_WORDS[COMP_CWORD-1]}"
  else
    previous=""
  fi

  commands="validate metaschema compile test fmt lint bundle inspect encode decode codegen install upgrade rdf llm version help"

  global_options="--verbose -v --resolve -r --default-dialect -d --json -j --http -h --debug -g --header -H --configuration -C --color"

  if [ "${COMP_CWORD}" -eq 1 ]
  then
    COMPREPLY=( $(compgen -W "${commands}" -- "${current}") )
    return 0
  fi

  if [ "${#COMP_WORDS[@]}" -lt 2 ]
  then
    return 0
  fi

  local command="${COMP_WORDS[1]}"

  case "${previous}" in
    --color)
      COMPREPLY=( $(compgen -W "auto always never" -- "${current}") )
      return 0
      ;;
    --extension|-e)
      COMPREPLY=( $(compgen -W ".json .yaml .yml" -- "${current}") )
      return 0
      ;;
    --resolve|-r|--ignore|-i|--configuration|-C)
      COMPREPLY=( $(compgen -f -d -- "${current}") )
      return 0
      ;;
    --default-dialect|-d)
      COMPREPLY=( $(compgen -W "https://json-schema.org/draft/2020-12/schema https://json-schema.org/draft/2019-09/schema https://json-schema.org/draft-07/schema https://json-schema.org/draft-06/schema https://json-schema.org/draft-04/schema https://json-schema.org/draft-03/schema" -- "${current}") )
      return 0
      ;;
    --header|-H)
      return 0
      ;;
    --indentation)
      COMPREPLY=( $(compgen -W "2 4 8" -- "${current}") )
      return 0
      ;;
    --jobs|-J)
      return 0
      ;;
    -n)
      if [ "${command}" = "fmt" ] || [ "${command}" = "lint" ]
      then
        COMPREPLY=( $(compgen -W "2 4 8" -- "${current}") )
      fi
      return 0
      ;;
    --loop|-l)
      return 0
      ;;
    --exclude|-x|--only|-o)
      return 0
      ;;
    --template)
      COMPREPLY=( $(compgen -f -X '!*.json' -- "${current}") )
      return 0
      ;;
    --compact)
      COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      return 0
      ;;
    -c)
      if [ "${command}" = "rdf" ]
      then
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
        return 0
      fi
      ;;
    --target)
      COMPREPLY=( $(compgen -W "typescript" -- "${current}") )
      return 0
      ;;
    --to)
      COMPREPLY=( $(compgen -W "draft4 draft6 draft7 2019-09 2020-12 openapi3.1 openapi3.2" -- "${current}") )
      return 0
      ;;
    --upgrade|-U)
      COMPREPLY=( $(compgen -W "draft4 draft6 draft7 2019-09 2020-12" -- "${current}") )
      return 0
      ;;
    --ask|--url|-u|--model|--param|--timeout|-T)
      return 0
      ;;
    -a)
      if [ "${command}" = "lint" ]
      then
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      return 0
      ;;
    -m)
      if [ "${command}" = "validate" ]
      then
        COMPREPLY=( $(compgen -f -X '!*.json' -- "${current}") )
      fi
      return 0
      ;;
    -p)
      return 0
      ;;
    -t)
      if [ "${command}" = "upgrade" ]
      then
        COMPREPLY=( $(compgen -W "draft4 draft6 draft7 2019-09 2020-12 openapi3.1 openapi3.2" -- "${current}") )
      elif [ "${command}" = "lint" ]
      then
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      elif [ "${command}" = "codegen" ]
      then
        COMPREPLY=( $(compgen -W "typescript" -- "${current}") )
      fi
      return 0
      ;;
  esac

  case "${command}" in
    validate)
      local options="--benchmark -b --loop -l --extension -e --ignore -i --trace -t --fast -f --template -m --valid -V --invalid -I"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml .jsonl) )
      fi
      ;;
    metaschema)
      local options="--extension -e --ignore -i --trace -t --continue -c"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    compile)
      local options="--extension -e --ignore -i --fast -f --minify -m"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    test)
      local options="--extension -e --ignore -i --jobs -J"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    fmt)
      local options="--check -c --extension -e --ignore -i --keep-ordering -k --indentation -n"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    lint)
      local options="--fix -f --extension -e --ignore -i --exclude -x --only -o --list -l --indentation -n --rule -a --top-level-rule -t"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    bundle)
      local options="--extension -e --ignore -i --without-id -w"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    inspect)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    encode)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .jsonl) )
      fi
      ;;
    decode)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.binpack' -- "${current}") )
      fi
      ;;
    codegen)
      local options="--name -n --target -t"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    install)
      local options="--force -f --frozen -z"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -d -- "${current}") )
      fi
      ;;
    upgrade)
      local options="--to -t"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    llm)
      local options="--ask -a --url -u --model -m --param -p --upgrade -U --timeout -T --raw -R --dry-run -D --trace -t --without-id -w"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    rdf)
      local options="--flatten -l --compact -c --fast -f --format-assertion -F --extension -e --ignore -i"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(_jsonschema_files "${current}" .json .yaml .yml) )
      fi
      ;;
    version|help)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      fi
      ;;
    *)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      fi
      ;;
  esac
}

complete -F _jsonschema jsonschema
