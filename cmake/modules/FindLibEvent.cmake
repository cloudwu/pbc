# - Find LibEvent (a cross platform RPC lib/tool)
# This module defines
#  LibEvent_INCLUDE_DIR, where to find LibEvent headers
#  LibEvent_LIBS, LibEvent libraries
#  LibEvent_FOUND, If false, do not try to use ant

set( INCS /usr/local/include /opt/local/include )
IF(MINGW)
    set( INCS ${INCS} /MinGW/include)
ENDIF(MINGW)

find_path(LibEvent_INCLUDE_DIR event2/event.h PATHS
     ${INCS}
  )

#message("X11_Xft_INCLUDE_PATH ${LibEvent_INCLUDE_DIR} ${INCS}" ) 

set(LibEvent_LIB_PATHS /usr/local/lib /opt/local/lib)
find_library(LibEvent_LIB NAMES event PATHS ${LibEvent_LIB_PATHS})

if (LibEvent_LIB AND LibEvent_INCLUDE_DIR)
  set(LibEvent_FOUND TRUE)
  set(LibEvent_LIBS ${LibEvent_LIB})
else ()
  set(LibEvent_FOUND FALSE)
endif ()

if (LibEvent_FOUND)
  if (NOT LibEvent_FIND_QUIETLY)
    message(STATUS "Found libevent: ${LibEvent_LIBS}")
  endif ()
else ()
  message(STATUS "libevent NOT found.")
endif ()

mark_as_advanced(
    LibEvent_LIB
    LibEvent_INCLUDE_DIR
  )
