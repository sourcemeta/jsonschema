if(TARGET termcolor::termcolor)
    return()
endif()

add_library(termcolor INTERFACE)
add_library(termcolor::termcolor ALIAS termcolor)

target_include_directories(termcolor INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/../vendorpool/termcolor/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TermColor
    REQUIRED_VARS CMAKE_CURRENT_LIST_DIR)