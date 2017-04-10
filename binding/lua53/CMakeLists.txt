aux_source_directory(${CMAKE_CURRENT_LIST_DIR} LIBSRCS)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")
if(CMAKE_OSX_ARCHITECTURES AND NOT ${CMAKE_OSX_ARCHITECTURES} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    add_library(protobuf STATIC ${LIBSRCS})
    target_link_libraries(protobuf ${LIBNAME})
else()
    add_library(protobuf SHARED ${LIBSRCS})
    target_link_libraries(protobuf ${LIBNAME} ${LUA_LIBRARIES})
endif()

file(GLOB LUA_CODE_LIST *.lua)
### install header
install(
    FILES protobuf.lua 
    DESTINATION "lib/lua")

### install library
install(TARGETS protobuf
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)