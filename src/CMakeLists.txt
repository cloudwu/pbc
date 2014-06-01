# ==== unittest TFoo ====

# FIX LUAJIT @MAC
IF(APPLE)
    SET(CMAKE_EXE_LINKER_FLAGS "-pagezero_size 10000 -image_base 100000000 ${CMAKE_EXE_LINKER_FLAGS}")
ENDIF(APPLE)

IF (WIN32)
     add_definitions(-DWIN32)   # how about x64?
     add_definitions(-TP) 
ELSE (WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF (WIN32)

set(CMAKE_C_FLAGS "${CMAKE_ENABLE_C99}")

SET(LIBPBC_SOURCE
    alloc.c
    array.c
    bootstrap.c
    context.c
    decode.c
    map.c
    pattern.c
    proto.c
    register.c
    rmessage.c
    stringpool.c
    varint.c
    wmessage.c
)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR})

#SET(LIBRARY_OUTPUT_PATH ../../lib/)
ADD_LIBRARY(pbc STATIC ${LIBPBC_SOURCE} )


ADD_EXECUTABLE(pbc_test pbc_test.c )
target_link_libraries(pbc_test pbc)

# the following use gtest
IF (WIN32)
  add_definitions(-DGTEST_HAS_TR1_TUPLE=0)
ENDIF (WIN32)

#INSTALL(TARGETS rosetta DESTINATION ${LIBRARY_INSTALL_PATH})

