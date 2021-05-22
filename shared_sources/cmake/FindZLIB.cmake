# - Find zlib
# Find the native ZLIB includes and library.
# Once done this will define
#
#  ZLIB_INCLUDE_DIRS   - where to find zlib.h, etc.
#  ZLIB_LIBRARIES      - List of libraries when using zlib.
#  ZLIB_FOUND          - True if zlib found.
#
#  ZLIB_VERSION_STRING - The version of zlib found (x.y.z)
#  ZLIB_VERSION_MAJOR  - The major version of zlib
#  ZLIB_VERSION_MINOR  - The minor version of zlib
#  ZLIB_VERSION_PATCH  - The patch version of zlib
#  ZLIB_VERSION_TWEAK  - The tweak version of zlib
#
# The following variable are provided for backward compatibility
#
#  ZLIB_MAJOR_VERSION  - The major version of zlib
#  ZLIB_MINOR_VERSION  - The minor version of zlib
#  ZLIB_PATCH_VERSION  - The patch version of zlib
#
# An includer may set ZLIB_ROOT to a zlib installation root to tell
# this module where to look.

SET(_ZLIB_SEARCHES)

# Search ZLIB_ROOT first if it is set.
IF(ZLIB_ROOT)
  SET(_ZLIB_SEARCH_ROOT PATHS ${ZLIB_ROOT} NO_DEFAULT_PATH)
  LIST(APPEND _ZLIB_SEARCHES _ZLIB_SEARCH_ROOT)
ENDIF()

# Normal search.
SET(_ZLIB_SEARCH_NORMAL
  PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\Zlib;InstallPath]"
#        "$ENV{PROGRAMFILES}/zlib" # Problem with x64 vs x86 !!!
  )
LIST(APPEND _ZLIB_SEARCHES _ZLIB_SEARCH_NORMAL)

# zlibstatic raised linkage errors... no idea why
#SET(ZLIB_NAMES z zlibstatic zlib zdll zlib1 zlibd zlibd1)
SET(ZLIB_NAMES z zlib32 zlib64 zlib zdll zlib1 zlibd zlibd1)

# Try each search configuration.
FOREACH(search ${_ZLIB_SEARCHES})
  FIND_PATH(ZLIB_INCLUDE_DIR NAMES zlib.h        ${${search}} PATH_SUFFIXES include)
  if(ARCH STREQUAL "x86")
    FIND_LIBRARY(ZLIB_LIBRARY  NAMES ${ZLIB_NAMES} ${${search}} PATH_SUFFIXES lib lib32)
  else()
    FIND_LIBRARY(ZLIB_LIBRARY  NAMES ${ZLIB_NAMES} ${${search}} PATH_SUFFIXES lib lib64)
  endif()
  if(WIN32)
    # want to find it later to copy it close to the exe files that need it
    # zlibstatic would avoid this but there is link error...
    if(ARCH STREQUAL "x86")
      FIND_PROGRAM(ZLIB_BIN  NAMES zlib.dll zlibd.dll ${${search}} PATH_SUFFIXES bin bin32)
    else()
      FIND_PROGRAM(ZLIB_BIN  NAMES zlib.dll zlibd.dll ${${search}} PATH_SUFFIXES bin bin64)
    endif()
  endif()
ENDFOREACH()

IF(ZLIB_INCLUDE_DIR AND EXISTS "${ZLIB_INCLUDE_DIR}/zlib.h")
    FILE(STRINGS "${ZLIB_INCLUDE_DIR}/zlib.h" ZLIB_H REGEX "^#define ZLIB_VERSION \"[^\"]*\"$")

    STRING(REGEX REPLACE "^.*ZLIB_VERSION \"([0-9]+).*$" "\\1" ZLIB_VERSION_MAJOR "${ZLIB_H}")
    STRING(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_MINOR  "${ZLIB_H}")
    STRING(REGEX REPLACE "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" ZLIB_VERSION_PATCH "${ZLIB_H}")
    SET(ZLIB_VERSION_STRING "${ZLIB_VERSION_MAJOR}.${ZLIB_VERSION_MINOR}.${ZLIB_VERSION_PATCH}")

    # only append a TWEAK version if it exists:
    SET(ZLIB_VERSION_TWEAK "")
    IF( "${ZLIB_H}" MATCHES "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*$")
        SET(ZLIB_VERSION_TWEAK "${CMAKE_MATCH_1}")
        SET(ZLIB_VERSION_STRING "${ZLIB_VERSION_STRING}.${ZLIB_VERSION_TWEAK}")
    ENDIF( "${ZLIB_H}" MATCHES "^.*ZLIB_VERSION \"[0-9]+\\.[0-9]+\\.[0-9]+\\.([0-9]+).*$")

    SET(ZLIB_MAJOR_VERSION "${ZLIB_VERSION_MAJOR}")
    SET(ZLIB_MINOR_VERSION "${ZLIB_VERSION_MINOR}")
    SET(ZLIB_PATCH_VERSION "${ZLIB_VERSION_PATCH}")
ENDIF()

# handle the QUIETLY and REQUIRED arguments and set ZLIB_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB REQUIRED_VARS ZLIB_LIBRARY ZLIB_INCLUDE_DIR
                                       VERSION_VAR ZLIB_VERSION_STRING)

IF(ZLIB_FOUND)
    #MARK_AS_ADVANCED(ZLIB_LIBRARY ZLIB_INCLUDE_DIR ZLIB_BIN)
    SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
    SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
ENDIF()

