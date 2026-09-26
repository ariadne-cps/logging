cmake_minimum_required(VERSION 3.15)

project(Logging VERSION 1.0)

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)


if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/configuration/cmake/ProjectOptions.cmake")
    message(FATAL_ERROR
        "Git submodules are not initialized. "
        "Run 'git submodule update --init --recursive' before configuring the project.")
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/submodules/configuration/cmake/ProjectOptions.cmake")

require_initialized_submodules()

option(THREAD_SANITIZER "Enable ThreadSanitizer" OFF)

setup_project()

if(COVERAGE AND THREAD_SANITIZER)
    message(FATAL_ERROR "COVERAGE and THREAD_SANITIZER cannot be enabled together.")
endif()

if(THREAD_SANITIZER)
    if(WIN32 OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "ThreadSanitizer is supported only with GCC or Clang on Unix-like systems.")
    endif()
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()

find_package(Threads REQUIRED)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/include)

if(NOT TARGET logging)
    add_subdirectory(src)

    add_library(logging ${LIBRARY_KIND} $<TARGET_OBJECTS:LOGGING_SRC>)
    set_target_properties(logging PROPERTIES OUTPUT_NAME "ariadne-logging")
    target_link_libraries(logging ${CMAKE_THREAD_LIBS_INIT})

    setup_standalone_project_tests(
        TARGET logging
        TEST_DIRECTORY test
        TEST_TARGET tests
        SOURCES
            "${PROJECT_SOURCE_DIR}/include"
            "${PROJECT_SOURCE_DIR}/src"
    )


endif()
