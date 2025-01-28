if(NOT Yaml_FOUND)
  set(YAML_DIR "${PROJECT_SOURCE_DIR}/vendor/yaml")
  set(YAML_PUBLIC_HEADER "${YAML_DIR}/include/yaml.h")

  set(YAML_SOURCES
    "${YAML_PUBLIC_HEADER}"
    "${YAML_DIR}/src/api.c"
    "${YAML_DIR}/src/dumper.c"
    "${YAML_DIR}/src/emitter.c"
    "${YAML_DIR}/src/loader.c"
    "${YAML_DIR}/src/parser.c"
    "${YAML_DIR}/src/reader.c"
    "${YAML_DIR}/src/scanner.c"
    "${YAML_DIR}/src/writer.c"
    "${YAML_DIR}/src/yaml_private.h")

  add_library(yaml ${YAML_SOURCES})

  if(SOURCEMETA_COMPILER_LLVM OR SOURCEMETA_COMPILER_GCC)
    target_compile_options(yaml PRIVATE -Wno-implicit-function-declaration)
    target_compile_options(yaml PRIVATE -Wno-int-to-pointer-cast)
  endif()

  if(LINUX)
    # See https://github.com/3DSGuy/Project_CTR/issues/122
    target_compile_definitions(yaml PRIVATE _GNU_SOURCE)
  endif()

  if(BUILD_SHARED_LIBS)
    target_compile_definitions(yaml PUBLIC YAML_DECLARE_EXPORT)
  else()
    target_compile_definitions(yaml PUBLIC YAML_DECLARE_STATIC)
  endif()

  target_include_directories(yaml PRIVATE "${YAML_DIR}/include")
  target_include_directories(yaml PUBLIC
    "$<BUILD_INTERFACE:${YAML_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

  target_compile_definitions(yaml PRIVATE YAML_VERSION_STRING="0.0.0")
  target_compile_definitions(yaml PRIVATE YAML_VERSION_MAJOR=0)
  target_compile_definitions(yaml PRIVATE YAML_VERSION_MINOR=0)
  target_compile_definitions(yaml PRIVATE YAML_VERSION_PATCH=0)

  set_target_properties(yaml
    PROPERTIES
      OUTPUT_NAME yaml
      PUBLIC_HEADER "${YAML_PUBLIC_HEADER}"
      C_VISIBILITY_PRESET "default"
      C_VISIBILITY_INLINES_HIDDEN FALSE
      EXPORT_NAME yaml)

  if(SOURCEMETA_CORE_INSTALL)
    include(GNUInstallDirs)
    install(TARGETS yaml
      EXPORT yaml
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        COMPONENT sourcemeta_core_dev
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT sourcemeta_core
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core
        NAMELINK_COMPONENT sourcemeta_core_dev
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT sourcemeta_core_dev)
    install(EXPORT yaml
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/yaml"
      NAMESPACE yaml::
      COMPONENT sourcemeta_core_dev)

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/yaml-config.cmake
      "include(\"\${CMAKE_CURRENT_LIST_DIR}/yaml.cmake\")\n"
      "check_required_components(\"yaml\")\n")
    install(FILES
      "${CMAKE_CURRENT_BINARY_DIR}/yaml-config.cmake"
      DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/yaml"
      COMPONENT sourcemeta_core_dev)
  endif()

  set(Yaml_FOUND ON)
endif()
