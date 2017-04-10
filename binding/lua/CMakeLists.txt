include_directories(${LUA_INCLUDE_DIR})

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} LIBSRCS)

if(CMAKE_OSX_ARCHITECTURES AND NOT LUA_LIBRARIES)
    add_library(protobuf STATIC ${LIBSRCS})
    target_link_libraries(protobuf ${LIBNAME})
else()
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
    set(CMAKE_STATIC_LIBRARY_PREFIX "")
    add_library(protobuf SHARED ${LIBSRCS})
    target_link_libraries(protobuf ${LIBNAME} ${LUA_LIBRARIES})
endif()

add_dependencies(lib protobuf)

file(GLOB LUA_CODE_LIST *.lua)
### install header
install(
    FILES protobuf.lua parser.lua
    DESTINATION "lib/lua")

### install library
install(TARGETS protobuf
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)