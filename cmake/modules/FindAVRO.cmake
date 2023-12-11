# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
AVRO
--------

Find the native AVRO includes and library.
Heavily inspired by the official FindZLIB cmake module v3.27.

IMPORTED Targets
^^^^^^^^^^^^^^^^

This module defines :prop_tgt:`IMPORTED` target ``AVRO::AVRO``, if
AVRO has been found.

Result Variables
^^^^^^^^^^^^^^^^

This module defines the following variables:

::

  AVRO_INCLUDE_DIRS   - where to find zip.h, unzip.h, etc.
  AVRO_LIBRARIES      - List of libraries when using avro.
  AVRO_FOUND          - True if avro found.

Hints
^^^^^

A user may set ``AVRO_ROOT`` to a avro installation root to tell this
module where to look.
#]=======================================================================]
include(SelectLibraryConfigurations)

set(_AVRO_SEARCHES)

# Search AVRO_ROOT first if it is set.
if(AVRO_ROOT)
  set(_AVRO_SEARCH_ROOT PATHS ${AVRO_ROOT} NO_DEFAULT_PATH)
  list(APPEND _AVRO_SEARCHES _AVRO_SEARCH_ROOT)
endif()

# Normal search.
set(_AVRO_x86 "(x86)")
set(_AVRO_SEARCH_NORMAL
    PATHS "$ENV{ProgramFiles}/avro"
          "$ENV{ProgramFiles${_AVRO_x86}}/avro")
unset(_AVRO_x86)
list(APPEND _AVRO_SEARCHES _AVRO_SEARCH_NORMAL)

set(AVRO_NAMES avrocpp avrocpp_s)
set(AVRO_NAMES_DEBUG avrod avrocpp_d avrocpp_s_d)

# Try each search configuration.
foreach(search ${_AVRO_SEARCHES})
  find_path(AVRO_INCLUDE_DIR NAMES avro/AvroParse.hh ${${search}} PATH_SUFFIXES include)
endforeach()

# Allow AVRO_LIBRARY to be set manually, as the location of the zlib library
if(NOT AVRO_LIBRARY)
  foreach(search ${_AVRO_SEARCHES})
    find_library(AVRO_LIBRARY_RELEASE NAMES ${AVRO_NAMES} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
    find_library(AVRO_LIBRARY_DEBUG NAMES ${AVRO_NAMES_DEBUG} NAMES_PER_DIR ${${search}} PATH_SUFFIXES lib)
  endforeach()

  select_library_configurations(AVRO)
endif()

unset(AVRO_NAMES)
unset(AVRO_NAMES_DEBUG)

mark_as_advanced(AVRO_INCLUDE_DIR)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(AVRO REQUIRED_VARS AVRO_LIBRARY AVRO_INCLUDE_DIR)

if(AVRO_FOUND)
    set(AVRO_INCLUDE_DIRS ${AVRO_INCLUDE_DIR})

    if(NOT AVRO_LIBRARIES)
      set(AVRO_LIBRARIES ${AVRO_LIBRARY})
    endif()

    if(NOT TARGET AVRO::AVRO)
      add_library(AVRO::AVRO UNKNOWN IMPORTED)
      set_target_properties(AVRO::AVRO PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${AVRO_INCLUDE_DIRS}")

      if(AVRO_LIBRARY_RELEASE)
        set_property(TARGET AVRO::AVRO APPEND PROPERTY
          IMPORTED_CONFIGURATIONS RELEASE)
        set_target_properties(AVRO::AVRO PROPERTIES
          IMPORTED_LOCATION_RELEASE "${AVRO_LIBRARY_RELEASE}")
      endif()

      if(AVRO_LIBRARY_DEBUG)
        set_property(TARGET AVRO::AVRO APPEND PROPERTY
          IMPORTED_CONFIGURATIONS DEBUG)
        set_target_properties(AVRO::AVRO PROPERTIES
          IMPORTED_LOCATION_DEBUG "${AVRO_LIBRARY_DEBUG}")
      endif()

      if(NOT AVRO_LIBRARY_RELEASE AND NOT AVRO_LIBRARY_DEBUG)
        set_property(TARGET AVRO::AVRO APPEND PROPERTY
          IMPORTED_LOCATION "${AVRO_LIBRARY}")
      endif()
    endif()
endif()
