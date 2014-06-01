# - Try to find QCA2 (Qt Cryptography Architecture 2)
# Once done this will define
#
#  QCA2_FOUND - system has QCA2
#  QCA2_INCLUDE_DIR - the QCA2 include directory
#  QCA2_LIBRARIES - the libraries needed to use QCA2
#  QCA2_DEFINITIONS - Compiler switches required for using QCA2
#
# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls

# Copyright (c) 2006, Michael Larouche, <michael.larouche@kdemail.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindLibraryWithDebug)

if (QCA2_INCLUDE_DIR AND QCA2_LIBRARIES)

  # in cache already
  set(QCA2_FOUND TRUE)

else (QCA2_INCLUDE_DIR AND QCA2_LIBRARIES)


  if (NOT WIN32)
    find_package(PkgConfig)
    pkg_check_modules(PC_QCA2 QUIET qca2)
    set(QCA2_DEFINITIONS ${PC_QCA2_CFLAGS_OTHER})
  endif (NOT WIN32)

  find_library(QCA2_LIBRARIES_DEBUG
                  NAMES qcad qcad2
                  HINTS ${PC_QCA2_LIBDIR} ${PC_QCA2_LIBRARY_DIRS}
                  )
  find_library(QCA2_LIBRARIES_RELEASE
                  NAMES qca qca2
                  HINTS ${PC_QCA2_LIBDIR} ${PC_QCA2_LIBRARY_DIRS}
                  )
  # if the release- as well as the debug-version of the library have been found:
  IF (QCA2_LIBRARIES_DEBUG AND QCA2_LIBRARIES_RELEASE)
    # if the generator supports configuration types then set
    # optimized and debug libraries, or if the CMAKE_BUILD_TYPE has a value
    IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      SET(QCA2_LIBRARIES       optimized ${QCA2_LIBRARIES_RELEASE} debug ${QCA2_LIBRARIES_DEBUG})
    ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
      # if there are no configuration types and CMAKE_BUILD_TYPE has no value
      # then just use the release libraries
      SET(QCA2_LIBRARIES       ${QCA2_LIBRARIES_RELEASE} )
    ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
  ELSE (QCA2_LIBRARIES_DEBUG AND QCA2_LIBRARIES_RELEASE)
    IF (QCA2_LIBRARIES_RELEASE)
       SET(QCA2_LIBRARIES ${QCA2_LIBRARIES_RELEASE})
    ENDIF (QCA2_LIBRARIES_RELEASE)
  ENDIF (QCA2_LIBRARIES_DEBUG AND QCA2_LIBRARIES_RELEASE)


  find_path(QCA2_INCLUDE_DIR qca.h
            HINTS ${PC_QCA2_INCLUDEDIR} ${PC_QCA2_INCLUDE_DIRS}
            PATH_SUFFIXES QtCrypto)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(QCA2 QCA2_LIBRARIES QCA2_INCLUDE_DIR)

  mark_as_advanced(QCA2_INCLUDE_DIR QCA2_LIBRARIES)

endif (QCA2_INCLUDE_DIR AND QCA2_LIBRARIES)
