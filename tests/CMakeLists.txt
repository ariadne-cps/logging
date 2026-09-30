include(CTest)

set(UNIT_TESTS
    test_logging
    test_concurrency
)

foreach(TEST ${UNIT_TESTS})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} ariadne-logging logging utility)
    add_test(NAME ${TEST} COMMAND $<TARGET_FILE:${TEST}>)
    if(WIN32)
        set_tests_properties(${TEST} PROPERTIES
            ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:ariadne-logging>"
        )
    endif()
endforeach()

add_custom_target(tests)
add_dependencies(tests ${UNIT_TESTS})
