cmake_minimum_required(VERSION 3.12.0)

# The NEW behavior for this policy is to set VERSION as documented by the project() command.
cmake_policy(SET CMP0048 NEW)

project(Fespp
	VERSION 2.3.0
	DESCRIPTION "Paraview plugin for importing EPC documents"
	HOMEPAGE_URL "https://github.com/F2I-Consulting/fespp"
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set (CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/install CACHE PATH "Fespp install prefix" FORCE)
endif (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# -----------------------------------------------------------------------------
# DEPENDENCIES
# -----------------------------------------------------------------------------

set (FESAPI_INCLUDE FESAPI_INCLUDE-NOTFOUND CACHE PATH "Path to the directory which contains the FESAPI header files")
set (FESAPI_LIBRARY_RELEASE FESAPI_LIBRARY_RELEASE-NOTFOUND CACHE FILEPATH "Path to the FESAPI release library")
set (FESAPI_LIBRARY_DEBUG FESAPI_LIBRARY_DEBUG-NOTFOUND CACHE FILEPATH "Path to the FESAPI debug library")

set (WITH_ETP OFF CACHE BOOL "Only if FESAPI build with ETP support... for building Fespp with ETP support.")
if (WITH_ETP)

	message ("WARNING: ETP support only if FESAPI build with ETP support")

	# -- Boost DEPENDENCY --
	set (BOOST_INCLUDE_DIR BOOST_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to the directory which contains the BOOST header files")
	if (NOT IS_DIRECTORY ${BOOST_INCLUDE_DIR})
		message(WARNING "The BOOST include dir (BOOST_INCLUDE_DIR variable) does not look to be a valid directory. Please modify it.")
	endif ()
	set (BOOST_SYSTEM_LIBRARY BOOST_SYSTEM_LIBRARY-NOTFOUND CACHE FILEPATH "Path to the file which contains the Boost system static library release")
	if (NOT EXISTS ${BOOST_SYSTEM_LIBRARY})
		message(WARNING "The BOOST library (BOOST_SYSTEM_LIBRARY variable) does not look to be a valid file. Please modify it.")
	endif ()	
	
	# -- avro DEPENDENCY --
	set (AVRO_INCLUDE_DIR AVRO_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to the directory which contains the avro header files")
	if (NOT IS_DIRECTORY ${AVRO_INCLUDE_DIR})
		message(WARNING "The Avro include dir (AVRO_INCLUDE_DIR variable) does not look to be a valid directory. Please modify it.")
	endif ()
	set (AVRO_LIBRARY_RELEASE AVRO_LIBRARY_RELEASE-NOTFOUND CACHE FILEPATH "Path to the file which contains the avro library release")
	if (NOT EXISTS ${AVRO_LIBRARY_RELEASE})
		message(WARNING "The avro library (AVRO_LIBRARY_RELEASE variable) does not look to be a valid file. Please modify it.")
	endif ()
	
endif (WITH_ETP)

add_subdirectory(src)
