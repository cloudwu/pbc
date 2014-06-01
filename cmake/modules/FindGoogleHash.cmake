# - Find GoogleHash
# Find the native Google Sparse Hash Etc includes
#
#  GoogleHash_INCLUDE_DIR - where to find sparse_hash_set, etc.
#  GoogleHash_FOUND       - True if GoogleHash found.


if (GoogleHash_INCLUDE_DIR)
  # Already in cache, be silent
  set(GoogleHash_FIND_QUIETLY TRUE)
endif ()

find_path(GoogleHash_INCLUDE_DIR google/sparse_hash_set
  /opt/local/include
  /usr/local/include
  /usr/include
)

if (GoogleHash_INCLUDE_DIR)
   set(GoogleHash_FOUND TRUE)
else ()
   set(GoogleHash_FOUND FALSE)
endif ()

if (GoogleHash_FOUND)
   if (NOT GoogleHash_FIND_QUIETLY)
      message(STATUS "Found GoogleHash: ${GoogleHash_INCLUDE_DIR}")
   endif ()
else ()
      message(STATUS "Not Found GoogleHash: ${GoogleHash_INCLUDE_DIR}")
   if (GoogleHash_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find GoogleHash includes")
   endif ()
endif ()

mark_as_advanced(
  GoogleHash_INCLUDE_DIR
)
