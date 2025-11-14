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

  commands="validate metaschema compile test fmt lint bundle inspect encode decode version help"

  global_options="--verbose -v --resolve -r --default-dialect -d --json -j --http -h"

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
    --extension|-e)
      COMPREPLY=( $(compgen -W ".json .yaml .yml" -- "${current}") )
      return 0
      ;;
    --resolve|-r|--ignore|-i)
      COMPREPLY=( $(compgen -f -d -- "${current}") )
      return 0
      ;;
    --default-dialect|-d)
      COMPREPLY=( $(compgen -W "https://json-schema.org/draft/2020-12/schema https://json-schema.org/draft/2019-09/schema https://json-schema.org/draft-07/schema https://json-schema.org/draft-06/schema https://json-schema.org/draft-04/schema" -- "${current}") )
      return 0
      ;;
    --indentation|-n)
      COMPREPLY=( $(compgen -W "2 4 8" -- "${current}") )
      return 0
      ;;
    --loop|-l)
      return 0
      ;;
    --exclude|-x|--only|-o)
      return 0
      ;;
    --template|-m)
      COMPREPLY=( $(compgen -f -X '!*.json' -- "${current}") )
      return 0
      ;;
  esac

  case "${command}" in
    validate)
      local options="--http -h --benchmark -b --loop -l --extension -e --ignore -i --trace -t --fast -f --template -m"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -X '!*.jsonl' -- "${current}") )
      fi
      ;;
    metaschema)
      local options="--http -h --extension -e --ignore -i --trace -t"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    compile)
      local options="--http -h --extension -e --ignore -i --fast -f --minify -m"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    test)
      local options="--http -h --extension -e --ignore -i"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    fmt)
      local options="--check -c --extension -e --ignore -i --keep-ordering -k --indentation -n"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    lint)
      local options="--fix -f --extension -e --ignore -i --exclude -x --only -o --list -l --strict -s --indentation -n"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    bundle)
      local options="--http -h --extension -e --ignore -i --without-id -w"
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${options} ${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    inspect)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.yaml' -X '!*.yml' -- "${current}") )
      fi
      ;;
    encode)
      if [[ ${current} == -* ]]
      then
        COMPREPLY=( $(compgen -W "${global_options}" -- "${current}") )
      else
        COMPREPLY=( $(compgen -f -X '!*.json' -X '!*.jsonl' -- "${current}") )
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
    version|help)
      COMPREPLY=()
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
