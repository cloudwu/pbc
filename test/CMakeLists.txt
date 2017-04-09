set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/test")

add_custom_target(run_test)

unset(TEST_PROTO_PBS)
## compile protocols
file(GLOB TEST_PROTO_LIST *.proto)
foreach(TEST_PROTO_FILE IN LISTS TEST_PROTO_LIST)
    get_filename_component(TEST_PROTO_BIN_NAME ${TEST_PROTO_FILE} NAME_WE)

    add_custom_command(OUTPUT "${EXECUTABLE_OUTPUT_PATH}/${TEST_PROTO_BIN_NAME}.pb"
        COMMAND ${Protobuf_PROTOC_EXECUTABLE} 
        -o "${EXECUTABLE_OUTPUT_PATH}/${TEST_PROTO_BIN_NAME}.pb" 
        --proto_path ${CMAKE_CURRENT_LIST_DIR} 
        ${TEST_PROTO_FILE}
        DEPENDS ${TEST_PROTO_FILE}
        COMMENT "Run: ${Protobuf_PROTOC_EXECUTABLE} -o ${TEST_PROTO_BIN_NAME}.pb -I ${CMAKE_CURRENT_LIST_DIR} ${TEST_PROTO_FILE}"
        WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    )

    list(APPEND TEST_PROTO_PBS "${EXECUTABLE_OUTPUT_PATH}/${TEST_PROTO_BIN_NAME}.pb")
endforeach()


## compile tests
file(GLOB TEST_SRC_LIST *.c)
include_directories("${CMAKE_CURRENT_LIST_DIR}/../src")
foreach(TEST_SRC_FILE IN LISTS TEST_SRC_LIST)
    get_filename_component(TEST_SRC_BIN_NAME ${TEST_SRC_FILE} NAME_WE)
    set(TEST_SRC_BIN_NAME "test_${TEST_SRC_BIN_NAME}")

    add_executable(${TEST_SRC_BIN_NAME} EXCLUDE_FROM_ALL ${TEST_SRC_FILE})
    target_link_libraries(${TEST_SRC_BIN_NAME} ${LIBNAME})

    if(MSVC)
        add_custom_target("run_${TEST_SRC_BIN_NAME}" COMMAND
            "${CMAKE_BUILD_TYPE}/${TEST_SRC_BIN_NAME}.exe"
            DEPENDS ${TEST_SRC_BIN_NAME} ${TEST_PROTO_PBS}
            COMMENT "Run Test: ${TEST_SRC_BIN_NAME}"
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
            USES_TERMINAL
        )

        add_dependencies(run_test "run_${TEST_SRC_BIN_NAME}")
    else()
        add_custom_target("run_${TEST_SRC_BIN_NAME}" COMMAND
            "./${TEST_SRC_BIN_NAME}"
            DEPENDS ${TEST_SRC_BIN_NAME} ${TEST_PROTO_PBS}
            COMMENT "Run Test: ${TEST_SRC_BIN_NAME}"
            WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
            USES_TERMINAL
        )

        add_dependencies(run_test "run_${TEST_SRC_BIN_NAME}")
    endif()
endforeach()