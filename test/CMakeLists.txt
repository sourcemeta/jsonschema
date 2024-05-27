macro(add_jsonschema_test_unix name)
  if(UNIX)
    add_test(NAME JSONSchema.${name} COMMAND
      "${CMAKE_CURRENT_SOURCE_DIR}/${name}.sh"
      "$<TARGET_FILE:jsonschema_cli>")
  endif()
endmacro()

add_jsonschema_test_unix(help)
add_jsonschema_test_unix(format_single)
add_jsonschema_test_unix(format_invalid_path)
add_jsonschema_test_unix(format_directory)
add_jsonschema_test_unix(format_cwd)
add_jsonschema_test_unix(frame)
add_jsonschema_test_unix(validate_pass)
add_jsonschema_test_unix(validate_fail)
add_jsonschema_test_unix(bundle_non_remote)
add_jsonschema_test_unix(test_single_pass)
add_jsonschema_test_unix(test_single_fail)
