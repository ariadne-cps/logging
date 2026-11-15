cmake_minimum_required(VERSION 3.15)

project(AriadneLogging VERSION 3.0)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(NOT TARGET ariadne-build)
    find_package(Git REQUIRED)
    # Keep initial Build checkout messages in English.
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env "LC_ALL=C"
                "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}"
                submodule update --init -- ss/bui
        RESULT_VARIABLE _result)
    if(_result)
        message(FATAL_ERROR "Cannot initialize build submodule")
    endif()
    include("${CMAKE_CURRENT_SOURCE_DIR}/ss/bui/cmake/ProjectOptions.cmake")
endif()

# Check only when Logging is the compilation root; nested consumers trust
# the pinned Logging commit and use the root's auxiliary dependencies.
require_same_dependency_commit(Build CHAINS
    "build" "utility|build" "testing|build")

find_package(Threads REQUIRED)
ariadne_setup_module(logging)

# Additional system dependency, not a Git submodule. This remains explicit.
target_link_libraries(logging INTERFACE Threads::Threads)

# Shared CTest registration policy for every module consuming Logging.
# The caller owns its --concise arguments and other test properties.
# Logging's own test suite uses plain add_test, so its output remains visible.
function(ariadne_add_silent_test)
    cmake_parse_arguments(SILENT_TEST "" "NAME" "" ${ARGN})
    if(NOT SILENT_TEST_NAME)
        message(FATAL_ERROR "ariadne_add_silent_test requires NAME")
    endif()
    add_test(${ARGN})
    set_property(TEST ${SILENT_TEST_NAME} APPEND PROPERTY ENVIRONMENT
                 "ARIADNE_TEST_LOG_NULL=1")
endfunction()
