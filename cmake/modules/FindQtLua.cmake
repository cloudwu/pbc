# - Find LibEvent (a cross platform RPC lib/tool)
# This module defines
#  LibEvent_INCLUDE_DIR, where to find LibEvent headers
#  LibEvent_LIBS, LibEvent libraries
#  LibEvent_FOUND, If false, do not try to use ant

set( INCS /usr/local/include /opt/local/include )
IF(MINGW)
    set( INCS ${INCS} /MinGW/include)
ENDIF(MINGW)

find_path(QtLua_INCLUDE_DIR QtLua/DispatchProxy PATHS
     ${INCS}
  )

#message("X11_Xft_INCLUDE_PATH ${LibEvent_INCLUDE_DIR} ${INCS}" ) 

set(QtLua_LIB_PATHS /usr/local/lib /opt/local/lib)
find_library(QtLua_LIB NAMES qtlua PATHS ${QtLua_LIB_PATHS})

if (QtLua_LIB AND QtLua_INCLUDE_DIR)
  set(QtLua_FOUND TRUE)
  set(QtLua_LIBS ${QtLua_LIB})
else ()
  set(QtLua_FOUND FALSE)
endif ()

if (QtLua_FOUND)
  if (NOT QtLua_FIND_QUIETLY)
    message(STATUS "Found libqtlua: ${QtLua_LIBS}")
  endif ()
else ()
  message(STATUS "libqtlua NOT found.")
endif ()

mark_as_advanced(
    QtLua_LIB
    QtLua_INCLUDE_DIR
  )
