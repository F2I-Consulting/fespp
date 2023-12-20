# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FESAPI
--------

Find the native FESAPI includes and library.
Heavily inspired by the official FindZLIB cmake module v3.27.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``FESAPI::FESAPI``, if
FESAPI has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  FESAPI_INCLUDE_DIRS   - where to find zip.h, unzip.h, etc.
  FESAPI_LIBRARIES      - List of libraries when using fesapi.
  FESAPI_FOUND          - True if fesapi found.

Hints
^^^^^

A user may set ``FESAPI_ROOT`` to a fesapi installation root to tell this
module where to look.
#]=======================================================================]
include(SelectLibraryConfigurations)

set(_FESAPI_SEARCHES)

# Search FESAPI_ROOT first if it is set.
if(FESAPI_ROOT)
  set(_FESAPI_SEARCH_ROOT PATHS ${FESAPI_ROOT} NO_DEFAULT_PATH)
  list(APPEND _FESAPI_SEARCHES _FESAPI_SEARCH_ROOT)
endif()

# Normal search.
set(_FESAPI_x86 "(x86)")
set(_FESAPI_SEARCH_NORMAL
    PATHS "$ENV{ProgramFiles}/fesapi"
          "$ENV{ProgramFiles${_FESAPI_x86}}/fesapi")
unset(_FESAPI_x86)
list(APPEND _FESAPI_SEARCHES _FESAPI_SEARCH_NORMAL)

# On Windows, library names contain the version
# The maximum of ranges are defined totally arbitrarily
set(FESAPI_NAMES FesapiCpp)
set(FESAPI_NAMES_DEBUG FesapiCppd)
foreach(minorVer RANGE 9 17)
	foreach(patchVer RANGE 0 5)
		foreach(tweakVer RANGE 0 5)
			list(APPEND FESAPI_NAMES FesapiCpp.2.${minorVer}.${patchVer}.${tweakVer})
			list(APPEND FESAPI_NAMES_DEBUG FesapiCppd.2.${minorVer}.${patchVer}.${tweakVer})
		endforeach()
	endforeach()
endforeach()

# Try each search configuration.
foreach(search ${_FESAPI_SEARCHES})
  find_path(FESAPI_INCLUDE_DIR NAMES fesapi/common/DataObjectRepository.h ${${search}} PATH_SUFFIXES include)
endforeach()

# Allow FESAPI_LIBRARY to be set manually, as the location of the zlib library
if(NOT FESAPI_LIBRARY)
  foreach(search ${_FESAPI_SEARCHES})
    find_library(FESAPI_LIBRARY_RELEASE NAMES ${FESAPI_NAMES} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
    find_library(FESAPI_LIBRARY_DEBUG NAMES ${FESAPI_NAMES_DEBUG} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
  endforeach()

  select_library_configurations(FESAPI)
endif()

unset(FESAPI_NAMES)
unset(FESAPI_NAMES_DEBUG)

mark_as_advanced(FESAPI_INCLUDE_DIR)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(FESAPI REQUIRED_VARS FESAPI_LIBRARY FESAPI_INCLUDE_DIR)

if(FESAPI_FOUND)
    set(FESAPI_INCLUDE_DIRS ${FESAPI_INCLUDE_DIR})

    if(NOT FESAPI_LIBRARIES)
      set(FESAPI_LIBRARIES ${FESAPI_LIBRARY})
    endif()

    if(NOT TARGET FESAPI::FESAPI)
      add_library(FESAPI::FESAPI UNKNOWN IMPORTED)
      set_target_properties(FESAPI::FESAPI PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FESAPI_INCLUDE_DIRS}")

      if(FESAPI_LIBRARY_RELEASE)
        set_property(TARGET FESAPI::FESAPI APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(FESAPI::FESAPI PROPERTIES
          IMPORTED_LOCATION_RELEASE "${FESAPI_LIBRARY_RELEASE}")
      endif()

      if(FESAPI_LIBRARY_DEBUG)
        set_property(TARGET FESAPI::FESAPI APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(FESAPI::FESAPI PROPERTIES
          IMPORTED_LOCATION_DEBUG "${FESAPI_LIBRARY_DEBUG}")
      endif()

      if(NOT FESAPI_LIBRARY_RELEASE AND NOT FESAPI_LIBRARY_DEBUG)
        set_property(TARGET FESAPI::FESAPI APPEND PROPERTY
          IMPORTED_LOCATION "${FESAPI_LIBRARY}")
      endif()
    endif()
endif()
