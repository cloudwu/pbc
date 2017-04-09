set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/test")

unset(TEST_PROTO_PBS)
## compile protocols
file(GLOB TEST_PROTO_LIST *.proto)
foreach(TEST_PROTO_FILE IN LISTS TEST_PROTO_LIST)
    get_filename_component(TEST_PROTO_BIN_NAME ${TEST_PROTO_FILE} NAME_WE)

    add_custom_command(OUTPUT "${TEST_PROTO_BIN_NAME}.pb" COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} 
        -o "${TEST_PROTO_BIN_NAME}.pb" 
        -I ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/${TEST_PROTO_FILE}"
        DEPENDS "${CMAKE_CURRENT_LIST_DIR}/${TEST_PROTO_FILE}"
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    )

    list(APPEND TEST_PROTO_PBS "${TEST_PROTO_BIN_NAME}.pb")
endforeach()


## compile tests
file(GLOB TEST_SRC_LIST *.c)
foreach(TEST_SRC_FILE IN LISTS TEST_SRC_LIST)
    get_filename_component(TEST_SRC_BIN_NAME ${TEST_SRC_FILE} NAME_WE)

    add_executable(${TEST_SRC_BIN_NAME} EXCLUDE_FROM_ALL ${TEST_SRC_FILE})
    target_link_libraries(${TEST_SRC_BIN_NAME} ${LIBNAME})

    add_dependencies(${TEST_SRC_BIN_NAME} ${TEST_PROTO_PBS})

    if(WIN32)
        add_test(NAME "run_test_${TEST_SRC_BIN_NAME}" COMMAND
            "${TEST_SRC_BIN_NAME}.exe"
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        )
    else()
        add_test(NAME "run_test_${TEST_SRC_BIN_NAME}" COMMAND
            "./${TEST_SRC_BIN_NAME}"
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
        )
    endif()
    add_dependencies("run_test_${TEST_SRC_BIN_NAME}" ${TEST_SRC_BIN_NAME})
endforeach()