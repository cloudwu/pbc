aux_source_directory(${CMAKE_CURRENT_LIST_DIR} LIBSRCS)

add_library(${LIBNAME} STATIC ${LIBSRCS})

### install header
install(FILES "${CMAKE_CURRENT_LIST_DIR}/../pbc.h" DESTINATION "include")

### install library
install(TARGETS ${LIBNAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)