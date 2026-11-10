cmake_minimum_required(VERSION 3.15)

project(Logging VERSION 3.0)

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
