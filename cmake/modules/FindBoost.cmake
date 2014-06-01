# - Find the Boost includes and libraries.
# The following variables are set if Boost is found.  If Boost is not
# found, Boost_FOUND is set to false.
#  Boost_FOUND        - True when the Boost include directory is found.
#  Boost_INCLUDE_DIRS - the path to where the boost include files are.
#  Boost_LIBRARY_DIRS - The path to where the boost library files are.
#  Boost_LIB_DIAGNOSTIC_DEFINITIONS - Only set if using Windows.
#  BOOST_LIBS - Our default boost libs
#  BOOST_THREAD_LIB - The name of the boost thread library
#  BOOST_PROGRAM_OPTIONS_LIB - The name of the boost program options library
#  BOOST_FILESYSTEM_LIB - The name of the boost filesystem library
#  BOOST_IOSTREAMS_LIB - The name of the boost program options library
#  BOOST_PYTHON_LIB - The name of the boost python library
#  BOOST_SYSTEM_LIB - The name of the boost system library
# ----------------------------------------------------------------------------
#
# Usage:
# In your CMakeLists.txt file do something like this:
# ...
# # Boost
# find_package(Boost)
# ...
# include_directories(${Boost_INCLUDE_DIRS})

if (NOT Boost_FIND_QUIETLY)
  message(STATUS "Looking for required boost libraries...")
endif ()

if (WIN32)
  set(Boost_LIB_DIAGNOSTIC_DEFINITIONS "-DBOOST_LIB_DIAGNOSTIC")
endif ()

set(BOOST_DIR_SEARCH $ENV{BOOST_ROOT})

if (BOOST_DIR_SEARCH)
  file(TO_CMAKE_PATH ${BOOST_DIR_SEARCH} BOOST_DIR_SEARCH)
  set(BOOST_DIR_SEARCH ${BOOST_DIR_SEARCH}/include)
endif ()

if (WIN32)
  set(BOOST_DIR_SEARCH
    ${BOOST_DIR_SEARCH}
    C:/boost/include
    D:/boost/include
    M:/Boost/include
  )
endif ()

# Add in some path suffixes. These will have to be updated whenever a new
# Boost version comes out.
set(SUFFIX_FOR_PATH
 boost-1_50
 boost-1_49
 boost-1_48
 boost-1_40
 boost-1_39
 boost-1_38
 boost-1_37
 boost-1_36_1
 boost-1_36_0
 boost-1_35_1
 boost-1_35_0
 boost-1_35
 boost-1_34_1
 boost-1_34_0
)

#
# Look for an installation.
#
find_path(Boost_INCLUDE_DIR
  NAMES boost/config.hpp
  PATH_SUFFIXES ${SUFFIX_FOR_PATH}
  # Look in other places.
  PATHS ${BOOST_DIR_SEARCH}
  # Help the user find it if we cannot.
  DOC "The ${BOOST_INCLUDE_PATH_DESCRIPTION}"
)

macro(FIND_BOOST_PARENT root includedir)
  # Look for the boost library path.
  # Note that the user may not have installed any libraries
  # so it is quite possible the library path may not exist.
  set(${root} ${includedir})

  if (${root} MATCHES "boost-[0-9]+")
    get_filename_component(${root} ${${root}} PATH)
  endif ()

  if (${root} MATCHES "/include$")
    # Strip off the trailing "/include" in the path.
    get_filename_component(${root} ${${root}} PATH)
  endif ()
endmacro(FIND_BOOST_PARENT root includedir)

macro(FIND_BOOST_LIBRARY lib libname libroot required)
  # libboost_chrono-mgw46-mt-1_49 -> patch for my mingw32
  set(${lib}_NAMES
    "boost_${libname}-mt"
    "libboost_${libname}-mgw46-mt-1_49"
    "boost_${libname}-gcc45-mt"
    "boost_${libname}-gcc44-mt"
    "boost_${libname}-gcc43-mt"
    "boost_${libname}-gcc42-mt"
    "boost_${libname}-gcc41-mt"
    "boost_${libname}-gcc34-mt"
    "boost_${libname}-xgcc40-mt"
  )
  if (NOT "${ARGN}" STREQUAL "MT_ONLY")
    set(${lib}_NAMES ${${lib}_NAMES} "boost_${libname}")
  endif ()

  find_library(${lib}
    NAMES ${${lib}_NAMES}
    PATHS "${libroot}/lib" "${libroot}/lib64"
  )
  if (required AND ${lib} MATCHES "NOTFOUND$")
    message(FATAL_ERROR "required boost library: ${lib} not found")
  endif ()

  mark_as_advanced(${lib})
endmacro()

# Assume we didn't find it.
set(Boost_FOUND 0)

# Now try to get the include and library path.
if (Boost_INCLUDE_DIR)
  try_run(BOOST_CHECK SHOULD_COMPILE
          ${ZygiellaQt_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp
          ${ZygiellaQt_SOURCE_DIR}/cmake/CheckBoost.cc
          CMAKE_FLAGS -DINCLUDE_DIRECTORIES:STRING=${Boost_INCLUDE_DIR}
          COMPILE_DEFINITIONS -v
          OUTPUT_VARIABLE BOOST_TRY_OUT)
  string(REGEX REPLACE ".*\n([0-9_]+).*" "\\1" BOOST_VERSION ${BOOST_TRY_OUT})
  string(REGEX REPLACE ".*\ngcc version ([0-9.]+).*" "\\1" GCC_VERSION
         ${BOOST_TRY_OUT})

  if (GCC_VERSION)
    message(STATUS "GCC version: ${GCC_VERSION}")
  endif ()

  message(STATUS "Boost version: ${BOOST_VERSION}")

  if (NOT BOOST_CHECK STREQUAL "0")
    message(FATAL_ERROR "Boost version not compatible")
  endif ()

  if (BOOST_VERSION MATCHES "1_34")
    set(Boost_HAS_SYSTEM_LIB false)
  else ()
    set(Boost_HAS_SYSTEM_LIB true)
  endif()

  message(STATUS "Boost include dir: ${Boost_INCLUDE_DIR}")
  FIND_BOOST_PARENT(Boost_PARENT ${Boost_INCLUDE_DIR})

  # Add boost libraries here
  FIND_BOOST_LIBRARY(BOOST_THREAD_LIB thread ${Boost_PARENT} true)
  get_filename_component(Boost_LIBRARY_DIR ${BOOST_THREAD_LIB} PATH)
  FIND_BOOST_LIBRARY(BOOST_PROGRAM_OPTIONS_LIB program_options
                     ${Boost_PARENT} true)
  FIND_BOOST_LIBRARY(BOOST_IOSTREAMS_LIB iostreams ${Boost_PARENT} true)
  FIND_BOOST_LIBRARY(BOOST_FILESYSTEM_LIB filesystem ${Boost_PARENT} true)
  FIND_BOOST_LIBRARY(BOOST_PYTHON_LIB python ${Boost_PARENT} false)
  FIND_BOOST_LIBRARY(BOOST_DATE_TIME_LIB date_time ${Boost_PARENT} false)

  if(Boost_HAS_SYSTEM_LIB)
    FIND_BOOST_LIBRARY(BOOST_SYSTEM_LIB system ${Boost_PARENT} true)
  endif()

  if (NOT Boost_FIND_QUIETLY)
    message(STATUS "Boost thread lib: ${BOOST_THREAD_LIB}")
    message(STATUS "Boost program options lib: ${BOOST_PROGRAM_OPTIONS_LIB}")
    message(STATUS "Boost filesystem lib: ${BOOST_FILESYSTEM_LIB}")
    message(STATUS "Boost iostreams lib: ${BOOST_IOSTREAMS_LIB}")
    message(STATUS "Boost python lib: ${BOOST_PYTHON_LIB}")
    message(STATUS "Boost date_time lib: ${BOOST_DATETIME_LIB}")

    if(Boost_HAS_SYSTEM_LIB)
      message(STATUS "Boost system lib: ${BOOST_SYSTEM_LIB}")
    endif()

    message(STATUS "Boost lib dir: ${Boost_LIBRARY_DIR}")
  endif ()

  # BOOST_LIBS is our default boost libs.
  set(BOOST_LIBS ${BOOST_IOSTREAMS_LIB} ${BOOST_PROGRAM_OPTIONS_LIB}
      ${BOOST_FILESYSTEM_LIB} ${BOOST_THREAD_LIB} ${BOOST_DATE_TIME_LIB})

  if(Boost_HAS_SYSTEM_LIB)
    set(BOOST_LIBS ${BOOST_LIBS} ${BOOST_SYSTEM_LIB})
  endif()

  if (EXISTS ${Boost_INCLUDE_DIR})
    set(Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIR})
    # We have found boost. It is possible that the user has not
    # compiled any libraries so we set Boost_FOUND to be true here.
    set(Boost_FOUND 1)
  endif ()

  if (Boost_LIBRARY_DIR AND EXISTS "${Boost_LIBRARY_DIR}")
    set(Boost_LIBRARY_DIRS ${Boost_LIBRARY_DIR})
  endif ()
endif ()

if (NOT Boost_FOUND)
  if (Boost_FIND_REQUIRED)
    message(FATAL_ERROR "Required boost libraries not found")
  elseif (NOT Boost_FIND_QUIETLY)
    message(STATUS "Required boost libraries not found")
  endif ()
endif ()

mark_as_advanced(
  Boost_INCLUDE_DIR
)
