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

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------

if (PARAVIEW_BUILD_QT_GUI)

	# -----------------------------------------------------------------------------
	# Identify all QT related source files
	# -----------------------------------------------------------------------------
	file (GLOB FESPP_PQ_HEADERS PQ*.h)
	file (GLOB FESPP_PQ_SOURCES PQ*.cxx)
	file (GLOB FESPP_TREE_HEADERS Tree*.h)
	file (GLOB FESPP_TREE_SOURCES Tree*.cxx)
	file (GLOB FESPP_FORMS *.ui)
	file (GLOB FESPP_RESOURCES Resources/*.qrc)
	if (NOT WITH_ETP)
		file (GLOB FESPP_PQ_HEADERS_ETP PQEtp*.h)
		file (GLOB FESPP_PQ_SOURCES_ETP PQEtp*.cxx)
		file (GLOB FESPP_FORMS_ETP PQEtp*.ui)
		file (GLOB FESPP_RESOURCES_ETP Resources/PQEtp*.qrc)
		list (REMOVE_ITEM FESPP_PQ_HEADERS ${FESPP_PQ_HEADERS_ETP})
		list (REMOVE_ITEM FESPP_PQ_SOURCES ${FESPP_PQ_SOURCES_ETP})
		list (REMOVE_ITEM FESPP_FORMS ${FESPP_FORMS_ETP})
		list (REMOVE_ITEM FESPP_RESOURCES ${FESPP_RESOURCES_ETP})
	endif ()

	if (WIN32)	
		find_package(Qt5 REQUIRED COMPONENTS Widgets)
	else (WIN32)
		find_package(Qt5 REQUIRED COMPONENTS Widgets X11Extras)
	endif (WIN32)

	paraview_plugin_add_dock_window(
		CLASS_NAME PQSelectionPanel
		INTERFACES IFACES
		SOURCES IFACE_SRCS
	)
	
	paraview_plugin_add_action_group (
		CLASS_NAME PQToolsActionGroup
		GROUP_NAME "ToolBar/Fespp"
		INTERFACES IFACES3
		SOURCES IFACE3_SRCS
	)
	
	set (interfaces
		${IFACES}
		${IFACES3}
	)
	set (FESPP_QT_SOURCES
		${FESPP_PQ_HEADERS}
		${FESPP_PQ_SOURCES}
		${FESPP_TREE_HEADERS}
		${FESPP_TREE_SOURCES}
		${IFACE_SRCS} 
		${IFACE3_SRCS}
	)
	set (sources
		${FESPP_NO_QT_SOURCES}
		${FESPP_QT_SOURCES})
	
	if (WITH_ETP)
		paraview_plugin_add_dock_window (
			CLASS_NAME PQEtpPanel
			INTERFACES IFACES4
			SOURCES IFACE4_SRCS
		)		
		list (APPEND interfaces ${IFACES4})
		list (APPEND FESPP_QT_SOURCES ${IFACE4_SRCS})
	endif ()
	
	paraview_add_plugin(${PROJECT_NAME}
		VERSION ${PROJECT_VERSION}
		UI_INTERFACES ${interfaces}
		UI_RESOURCES ${FESPP_RESOURCES}
		UI_FILES ${FESPP_FORMS}
		SOURCES ${FESPP_QT_SOURCES}
		#SERVER_MANAGER_XML EPCReader.xml
		MODULES EPCReader
		MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/vtk.module"
		XML_DOCUMENTATION OFF
	)

else (PARAVIEW_BUILD_QT_GUI)

	# create a Server plugin
	paraview_add_plugin(${PROJECT_NAME}
		REQUIRED_ON_SERVER
		VERSION ${PROJECT_VERSION}
		SOURCES ${FESPP_QT_SOURCES}
		#SERVER_MANAGER_XML EPCReader.xml
		MODULES EPCReader
		MODULE_FILES EPCReader/vtk.module
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
			$<$<CONFIG:Debug>:"C:/pv57/pvbd/bin/paraview-5.7/plugins/Fespp"> $<$<CONFIG:RelWithDebInfo>:"C:/pv57/install_x64_vs2017/bin/paraview-5.7/plugins/Fespp">
	)
endif (DEV_UNDER_WIN32)
