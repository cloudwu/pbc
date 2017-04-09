cmake_minimum_required(VERSION 3.1.0)
cmake_policy(SET CMP0054 NEW)

enable_testing()
project(pbc C)

#####################################################################
# output
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

# compiler
if( ${CMAKE_C_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    add_definitions(-Wall)

    if(NOT WIN32 AND NOT CYGWIN AND NOT MINGW) 
        add_definitions(-fPIC)
    endif()

    # just for gcc
    if( ${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
        include(CheckCCompilerFlag)
        message(STATUS "Check Flag: -rdynamic -- running")
        CHECK_C_COMPILER_FLAG(-rdynamic, C_FLAGS_RDYNAMIC_AVAILABLE)
        if(C_FLAGS_RDYNAMIC_AVAILABLE)
            message(STATUS "Check Flag: -rdynamic -- yes")
            add_definitions(-rdynamic)
        else()
            message(STATUS "Check Flag: -rdynamic -- no")
        endif()

        # gcc 4.9 and output with color
        if ( CMAKE_C_COMPILER_VERSION VERSION_GREATER "4.9.0" OR CMAKE_C_COMPILER_VERSION  VERSION_EQUAL "4.9.0" )
            add_definitions(-fdiagnostics-color=auto)
        endif()
    endif()
endif()

# Compile flags
if ( NOT MSVC )
    list(APPEND CMAKE_C_FLAGS_DEBUG -ggdb -O0)
    #list(APPEND CMAKE_C_FLAGS_RELEASE)
    list(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO -ggdb)
    #list(APPEND CMAKE_C_FLAGS_MINSIZEREL)
else()
    if(NOT CMAKE_MSVC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME "MD")
    endif()
    list(APPEND CMAKE_C_FLAGS_DEBUG /Od /${CMAKE_MSVC_RUNTIME}d)
    list(APPEND CMAKE_C_FLAGS_RELEASE /O2 /${CMAKE_MSVC_RUNTIME} /D NDEBUG)
    list(APPEND CMAKE_C_FLAGS_RELWITHDEBINFO /O2 /${CMAKE_MSVC_RUNTIME}d)
    list(APPEND CMAKE_C_FLAGS_MINSIZEREL /Ox /${CMAKE_MSVC_RUNTIME} /D NDEBUG)
endif()

# add custom define
macro(add_compiler_define)
	foreach(def ${ARGV})
    	if ( NOT MSVC )
            add_definitions(-D${def})
        else()
            add_definitions("/D ${def}")
        endif()
	endforeach()
endmacro(add_compiler_define)


if(NOT CMAKE_BUILD_TYPE)
	#set(CMAKE_BUILD_TYPE "Debug")
	set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()

########################################################################
##
## archives and executable files
## 
########################################################################

include_directories(${CMAKE_CURRENT_LIST_DIR})

## libpbc.a
set(LIBNAME "pbc")
add_subdirectory(src)

add_custom_target(lib ALL DEPENDS ${LIBNAME})

## tool
add_subdirectory(tool)

## test
find_package(Protobuf)
if (Protobuf_FOUND)
    message(STATUS "Test: protobuf found at ${Protobuf_PROTOC_EXECUTABLE}, enable test sub-project now.")
    add_subdirectory(test)
endif()


## Lua - binding
find_package(Lua)
if (LUA_FOUND)
    message(STATUS "Lua: ${LUA_VERSION_STRING} found.INC=${LUA_INCLUDE_DIR},EXE=${LUA_LIBRARIES}")
endif()
## Lua 5.1
## LuaJIT
## Lua 5.3