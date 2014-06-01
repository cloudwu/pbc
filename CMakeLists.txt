# Project
project(pbc)
set(MAJOR_VERSION 1)
set(MINOR_VERSION 1)
set(PACKAGE pbc)
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION})
set(VERSION ${VERSION_STRING})

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(test "Build all tests." ON) # Makes boolean 'test' available.


if(COMMAND cmake_policy)
   cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH} )

########################################################################
## look for unordered set implementation

IF(CMAKE_COMPILER_IS_GNUCC)
  IF(NOT USE_CLANG)
    # Only gcc version > 4.0.1 have const-correct tr1::unordered_set
    # see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=23053
    EXEC_PROGRAM(${CMAKE_CXX_COMPILER} ARGS -dumpversion OUTPUT_VARIABLE cxx_COMPILER_VERSION)
    IF(${cxx_COMPILER_VERSION} VERSION_GREATER 4.0.1)
      SET(HAVE_TR1_UNORDERED_SET 1)
      SET(UNORDERED_SET_USES_TR1_UNORDERED_SET 1)
      #ADD_DEFINITIONS(-DUNORDERED_SET_USES_TR1_UNORDERED_SET)
    ELSE()
      SET(UNORDERED_SET_USES_EXT_HASH_SET 1)
    ENDIF()
  ENDIF()
ENDIF(CMAKE_COMPILER_IS_GNUCC)

IF(USE_CLANG)
  SET(MIN_CLANG_VERSION_FOR_TR1_UNORDERED_SET 2.8)  #at least 2.9
  # It seems that version number is not the same on MacOSX
  IF (APPLE)
    SET(MIN_CLANG_VERSION_FOR_TR1_UNORDERED_SET 2.0) #at least 2.1
  ENDIF()
   IF(${CLANG_VERSION} VERSION_GREATER ${MIN_CLANG_VERSION_FOR_TR1_UNORDERED_SET})
      SET(HAVE_TR1_UNORDERED_SET 1)
      SET(UNORDERED_SET_USES_TR1_UNORDERED_SET 1)
   ENDIF()
ENDIF(USE_CLANG)

# CMake
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
   set(CMAKE_BUILD_TYPE     RelWithDebInfo)
endif("${CMAKE_BUILD_TYPE}" STREQUAL "")
set(CMAKE_COLOR_MAKEFILE ON)
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

IF (WIN32)
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )  
ENDIF (WIN32)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )  # python & javaext all needs dll in bin


# Debug build
message("-- Build type: ${CMAKE_BUILD_TYPE}")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
   add_definitions(-DDEBUG)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

#include the subdirectory containing our libs
add_subdirectory (src)

# install (TARGETS CMakeHelloWorld DESTINATION bin)
