# - Find Zookeeper (a cross platform RPC lib/tool)
# This module defines
#  Zookeeper_INCLUDE_DIR, where to find Zookeeper headers
#  Zookeeper_LIBS, Zookeeper libraries
#  Zookeeper_FOUND, If false, do not try to use ant

find_path(Zookeeper_INCLUDE_DIR zookeeper.h PATHS
	/usr/local/include
	/opt/tdb-dev/include
)

set(Zookeeper_LIB_PATHS /usr/local/lib /opt/local/lib)

find_library(Zookeeper_LIB NAMES zookeeper_mt PATHS ${Zookeeper_LIB_PATHS})

if (Zookeeper_LIB AND Zookeeper_INCLUDE_DIR)
  set(Zookeeper_FOUND TRUE)
  set(Zookeeper_LIBS ${Zookeeper_LIB})
else ()
  set(Zookeeper_FOUND FALSE)
  message(STATUS "zookeeper is not found")
endif ()

if (Zookeeper_FOUND)
  if (NOT Zookeeper_FIND_QUIETLY)
    message(STATUS "Found zookeeper: ${Zookeeper_LIBS}")
  endif ()
else ()
  message(STATUS "Zookeeper libraries NOT found. "
          "Zookeeper support will be disabled (${Zookeeper_RETURN}, "
          "${Zookeeper_INCLUDE_DIR}, ${Zookeeper_LIB})")
endif ()

mark_as_advanced(
  Zookeeper_LIB
  Zookeeper_INCLUDE_DIR
  )
