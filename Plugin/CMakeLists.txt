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

find_package(FESAPI REQUIRED)

set (WITH_ETP_SSL OFF CACHE BOOL "Only if FESAPI build with ETP support... for building Fespp with ETP support.")

if (WITH_ETP_SSL)
	find_package(Boost 1.70)
	if (NOT Boost_FOUND)
		# Boost system is required for Beast untill version 1.70 : https://www.boost.org/doc/libs/1_69_0/libs/beast/doc/html/beast/introduction.html
		find_package(Boost 1.66 REQUIRED system)
	endif()
	find_package( Threads )
	if (NOT TARGET OpenSSL::SSL OR NOT TARGET OpenSSL::Crypto)
		find_package(OpenSSL REQUIRED)
	endif()
	
	#FETPAPI
	set (FETPAPI_INCLUDE_DIR FETPAPI_INCLUDE_DIR-NOTFOUND CACHE PATH "Path to the directory which contains the FETPAPI header files")
	if (NOT IS_DIRECTORY ${FETPAPI_INCLUDE_DIR})
		message(WARNING "The FETPAPI include dir (FETPAPI_INCLUDE_DIR variable) does not look to be a valid directory. Please modify it.")
	endif ()
	set (FETPAPI_LIBRARY_RELEASE FETPAPI_LIBRARY_RELEASE-NOTFOUND CACHE FILEPATH "Path to the file which contains the FETPAPI library release")
	if (NOT EXISTS ${FETPAPI_LIBRARY_RELEASE})
		message(WARNING "The FETPAPI library (FETPAPI_LIBRARY_RELEASE variable) does not look to be a valid file. Please modify it.")
	endif ()
	set (FETPAPI_LIBRARY_DEBUG FETPAPI_LIBRARY_DEBUG-NOTFOUND CACHE FILEPATH "Path to the file which contains the FETPAPI library DEBUG")
	if (NOT EXISTS ${FETPAPI_LIBRARY_DEBUG})
		message(WARNING "The FETPAPI library (FETPAPI_LIBRARY_DEBUG variable) does not look to be a valid file. Please modify it.")
	endif ()

	#AVRO
	find_package(AVRO REQUIRED)
endif (WITH_ETP_SSL)

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------

if (PARAVIEW_BUILD_QT_GUI)

	# -----------------------------------------------------------------------------
	# Identify all QT related source files
	# -----------------------------------------------------------------------------
	if (WIN32)	
		find_package(Qt5 REQUIRED COMPONENTS Widgets)
	else (WIN32)
		find_package(Qt5 REQUIRED COMPONENTS Widgets X11Extras)
	endif (WIN32)
	
	paraview_add_plugin(${PROJECT_NAME}
		VERSION ${PROJECT_VERSION}
		SOURCES ${FESPP_NO_QT_SOURCES}
		MODULES Energistics
		MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Energistics/vtk.module"
		XML_DOCUMENTATION OFF
	)

else (PARAVIEW_BUILD_QT_GUI)

	# create a Server plugin
	paraview_add_plugin(${PROJECT_NAME}
		REQUIRED_ON_SERVER
		VERSION ${PROJECT_VERSION}
		SOURCES ${FESPP_NO_QT_SOURCES}
		MODULES Energistics
		MODULE_FILES Energistics/vtk.module
		XML_DOCUMENTATION OFF)

endif (PARAVIEW_BUILD_QT_GUI)

# ============================================================================
# Compile definitions
# ============================================================================

# -----------------------------------------------------------------------------
# Disable deprecation warnings for standard C and STL functions in VS2005 and
# later (no, we don't need IF(CMAKE_COMPILER_2005) ... )
# -----------------------------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_DEPRECATE>
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_DEPRECATE>
	$<$<CXX_COMPILER_ID:MSVC>:_SCL_SECURE_NO_DEPRECATE>
)

# ============================================================================
# compilation options
# ============================================================================

target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/MP>
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<CXX_COMPILER_ID:GNU>:-Wall>
	$<$<CXX_COMPILER_ID:GNU>:-Wextra>
	$<$<CXX_COMPILER_ID:GNU>:-pedantic>
	$<$<CXX_COMPILER_ID:CLANG>:-Weverything>
)

if (WIN32)

	if (NOT EXISTS ${FESAPI_LIBRARY_DEBUG})
		set(CMAKE_CONFIGURATION_TYPES "Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
	endif ()
	if (NOT EXISTS ${FESAPI_LIBRARY_RELEASE})
		set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
	endif ()
	
elseif (UNIX)

	if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
    endif()
	
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
	)
	
endif (WIN32)

# ============================================================================
# Win32 Visual Studio file hierarchy
# ============================================================================

option(DEV_UNDER_WIN32 "Enable copy of the build in a Paraview install directory" OFF)

if (DEV_UNDER_WIN32)
	add_custom_command(
		TARGET Fespp 
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			$<$<CONFIG:Debug>:$<TARGET_FILE:Fespp>> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:Fespp>>
			$<$<CONFIG:RelWithDebInfo>:$<TARGET_FILE:Fespp>> $<$<CONFIG:RelWithDebInfo>:$<TARGET_PDB_FILE:Fespp>>
			$<$<CONFIG:Debug>:"${ParaView_DIR}/bin/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}/plugins/Fespp"> $<$<CONFIG:RelWithDebInfo>:"${ParaView_DIR}/bin/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}/plugins/Fespp">
	)
endif (DEV_UNDER_WIN32)
