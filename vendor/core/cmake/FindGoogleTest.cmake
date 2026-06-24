if(NOT GoogleTest_FOUND)
  set(BUILD_GMOCK ON CACHE BOOL "enable googlemock")
  set(INSTALL_GTEST OFF CACHE BOOL "disable installation")
  add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/googletest")
  set(GoogleTest_FOUND ON)

  # GoogleTest is vendored as-is and is not held to this project's strict
  # warning policy, so do not let its own diagnostics fail the build
  foreach(googletest_target gtest gtest_main gmock gmock_main)
    if(TARGET ${googletest_target})
      set_target_properties("${googletest_target}"
        PROPERTIES COMPILE_WARNING_AS_ERROR OFF)
    endif()
  endforeach()
endif()
