# Since Visual Studio 2005, you get a bunch of warnings when using
# strncpy. Make it quiet !
IF(WIN32)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
ENDIF(WIN32)

# Use this on platforms where dlopen() is in -ldl
IF (HAVE_LDL)
  SET(EXTRA_LIBS "dl")
ENDIF (HAVE_LDL)

IF (CMAKE_COMPILER_IS_GNUCC)
  SET(CMAKE_C_FLAGS_DEBUG "-Wall ${CMAKE_C_FLAGS_DEBUG}")
ENDIF (CMAKE_COMPILER_IS_GNUCC)
