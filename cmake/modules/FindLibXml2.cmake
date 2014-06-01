# - Find LibXml2
# This module defines
#  LibXml2_LIBS, LibXml2 libraries
#  LibXml2_FOUND, If false, do not try to use ant

#find_path(TDB_INCLUDE_DIR tdb.h PATHS
#    /usr/local/include
#    /opt/tdb-dev/include
#  )

set(LibXml2_LIB_PATHS /usr/local/lib /opt/tdb-dev/lib)
find_library(LIBXML2_LIB NAMES xml2 PATHS ${LIBXML2_LIB_PATHS})

if (LIBXML2_LIB)
  set(LIBXML2_FOUND TRUE)
  set(LIBXML2_LIBS ${LIBXML2_LIB})
else ()
  set(LIBXML2_FOUND FALSE)
endif ()

if (LIBXML2_FOUND)
  if (NOT LIBXML2_FIND_QUIETLY)
    message(STATUS "Found LibXml2: ${LIBXML2_LIBS}")
  endif ()
else ()
  message(STATUS "LibXml2 NOT found.")
endif ()

mark_as_advanced(
    LIBXML2_LIB
)
