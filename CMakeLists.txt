cmake_minimum_required(VERSION 3.15)

project(Logging VERSION 1.0)

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

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    require_same_dependency_commit(Build CHAINS
        "build"
        "utility|build"
        "testing|build")
endif()


setup_project()

find_package(Threads REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(ss)

if(NOT TARGET logging)
    add_library(logging INTERFACE)
    target_include_directories(logging INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/ariadne-logging>
    )
    target_link_libraries(logging INTERFACE utility Threads::Threads)

    add_subdirectory(src)
    target_link_libraries(LOGGING_SRC PRIVATE logging)
endif()

ariadne_register_public_headers(
    TARGET logging
    DIRECTORIES "${PROJECT_SOURCE_DIR}/include/logging"
    FILES "${PROJECT_SOURCE_DIR}/include/ariadne-logging.hpp"
)

setup_project_library(
    TARGET ariadne-logging
    OBJECTS LOGGING_SRC
    LINK_LIBRARIES logging
)

setup_standalone_project_tests(
    TARGET ariadne-logging
    TEST_DIRECTORY tests
    TEST_TARGET tests
    SOURCES
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/src"
)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    ariadne_install_dependency_bundle(
        TARGET logging
        DESTINATION include/ariadne-logging
    )
endif()
