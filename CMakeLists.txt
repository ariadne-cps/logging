cmake_minimum_required(VERSION 3.15)

project(Logging VERSION 1.0)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ss/configuration/cmake/ProjectOptions.cmake")
    message(FATAL_ERROR
        "Git submodules are not initialized. "
        "Run 'git submodule update --init --recursive' before configuring the project.")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/ss/configuration/cmake/ProjectOptions.cmake")

require_initialized_submodules()

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    require_same_dependency_commit(Configuration
        REFERENCES
            "${PROJECT_SOURCE_DIR}|ss/configuration"
            "${PROJECT_SOURCE_DIR}/ss/utility|ss/configuration"
            "${PROJECT_SOURCE_DIR}/ss/utility/ss/macro|ss/configuration"
    )
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
