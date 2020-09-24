# -----------------------------------------------------------------------------
# Identify all non QT source files
# -----------------------------------------------------------------------------
FILE (GLOB FESPP_VTK_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/VTK/*.cxx)
FILE (GLOB FESPP_VTK_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/VTK/*.h)

if (WITH_ETP)
	FILE (GLOB FESPP_ETP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/etp/*.cxx)
	FILE (GLOB FESPP_ETP_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/etp/*.h)
ENDIF (WITH_ETP)

set (FESPP_NO_QT_SOURCES ${FESPP_VTK_SOURCES} ${FESPP_VTK_HEADERS} ${FESPP_ETP_SOURCES} ${FESPP_ETP_HEADERS})

# -----------------------------------------------------------------------------
# Build the Paraview plugins
# -----------------------------------------------------------------------------

find_package(ParaView REQUIRED)
include ("${PARAVIEW_USE_FILE}")

if (PARAVIEW_BUILD_QT_GUI)

	# -----------------------------------------------------------------------------
	# Identify all QT related source files
	# -----------------------------------------------------------------------------
	file (GLOB FESPP_PQ_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/PQ*.h)
	file (GLOB FESPP_PQ_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/PQ*.cxx)
	file (GLOB FESPP_TREE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/Tree*.h)
	file (GLOB FESPP_TREE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Tree*.cxx)
	file (GLOB FESPP_FORMS ${CMAKE_CURRENT_SOURCE_DIR}/*.ui)
	file (GLOB FESPP_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/*.qrc)
	if (NOT WITH_ETP)
		file (GLOB FESPP_PQ_HEADERS_ETP ${CMAKE_CURRENT_SOURCE_DIR}/PQEtp*.h)
		file (GLOB FESPP_PQ_SOURCES_ETP ${CMAKE_CURRENT_SOURCE_DIR}/PQEtp*.cxx)
		file (GLOB FESPP_FORMS_ETP ${CMAKE_CURRENT_SOURCE_DIR}/PQEtp*.ui)
		file (GLOB FESPP_RESOURCES_ETP ${CMAKE_CURRENT_SOURCE_DIR}/Resources/PQEtp*.qrc)
		list (REMOVE_ITEM FESPP_PQ_HEADERS ${FESPP_PQ_HEADERS_ETP})
		list (REMOVE_ITEM FESPP_PQ_SOURCES ${FESPP_PQ_SOURCES_ETP})
		list (REMOVE_ITEM FESPP_FORMS ${FESPP_FORMS_ETP})
		list (REMOVE_ITEM FESPP_RESOURCES ${FESPP_RESOURCES_ETP})
	endif ()
	
	if (WIN32)	
		pv_find_package_qt(qt_targets REQUIRED QT5_COMPONENTS Widgets)
	else (WIN32)
		pv_find_package_qt(qt_targets REQUIRED QT5_COMPONENTS Widgets X11Extras)
	endif (WIN32)
	pv_qt_wrap_cpp(MOC_SRCS ${FESPP_PQ_HEADERS})
	pv_qt_wrap_ui(UI_SRCS ${FESPP_FORMS})

	ADD_PARAVIEW_DOCK_WINDOW(
		IFACES
		IFACE_SRCS
		CLASS_NAME PQSelectionPanel
	)
		
	ADD_PARAVIEW_ACTION_GROUP(
		IFACES3
		IFACE3_SRCS
		CLASS_NAME PQToolsActionGroup
		GROUP_NAME "ToolBar/EPC"
	)
	
	set (FESPP_IFACES
		${IFACES}
		${IFACES3}
	)
	set (FESPP_QT_SOURCES
		${FESPP_PQ_HEADERS}
		${FESPP_PQ_SOURCES}
		${FESPP_TREE_HEADERS}
		${FESPP_TREE_SOURCES}
		${MOC_SRCS}
		${UI_SRCS}
		${IFACE_SRCS} 
		${IFACE3_SRCS}
	)
	if (WITH_ETP)
		ADD_PARAVIEW_DOCK_WINDOW(
			IFACES4
			IFACE4_SRCS
			CLASS_NAME PQEtpPanel
		)		
		list (APPEND FESPP_IFACES ${IFACES4})
		list (APPEND FESPP_QT_SOURCES ${IFACE4_SRCS})
	endif ()
	
	ADD_PARAVIEW_PLUGIN(${PROJECT_NAME} ${PROJECT_VERSION}
		DOCUMENTATION_DIR "${CMAKE_SOURCE_DIR}/doc"
		SERVER_MANAGER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/vtkEPCReader.cxx
		SERVER_MANAGER_XML ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/EPCReader.xml
		GUI_INTERFACES ${FESPP_IFACES}
		GUI_RESOURCES ${FESPP_RESOURCES}
	)
	target_sources(${PROJECT_NAME} PRIVATE ${FESPP_QT_SOURCES})

else (PARAVIEW_BUILD_QT_GUI)

	# create a Server plugin
	ADD_PARAVIEW_PLUGIN(${PROJECT_NAME} ${PROJECT_VERSION}
		DOCUMENTATION_DIR "${CMAKE_SOURCE_DIR}/doc"
		SERVER_MANAGER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/vtkEPCReader.cxx
		SERVER_MANAGER_XML ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader/EPCReader.xml
		REQUIRED_ON_SERVER
	)

endif (PARAVIEW_BUILD_QT_GUI)

target_sources(${PROJECT_NAME} PRIVATE ${FESPP_NO_QT_SOURCES})

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
if (WITH_ETP)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		WITH_ETP
		BOOST_ALL_NO_LIB
	)
endif (WITH_ETP)

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

# ============================================================================
# External include
# ============================================================================

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/EPCReader)

target_include_directories(${PROJECT_NAME} PRIVATE ${FESAPI_INCLUDE})

if (WITH_ETP)
	target_include_directories(${PROJECT_NAME} PRIVATE ${AVRO_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
endif (WITH_ETP)

# ============================================================================
# External libraries
# ============================================================================

if (WIN32)

	if (EXISTS ${FESAPI_LIBRARY_RELEASE} AND EXISTS ${FESAPI_LIBRARY_DEBUG})
		set(CMAKE_CONFIGURATION_TYPES "Release;MinSizeRel;RelWithDebInfo;Debug" CACHE STRING "" FORCE)
	endif ()
	if (NOT EXISTS ${FESAPI_LIBRARY_DEBUG})
		set(CMAKE_CONFIGURATION_TYPES "Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
	endif ()
	if (NOT EXISTS ${FESAPI_LIBRARY_RELEASE})
		set(CMAKE_CONFIGURATION_TYPES "Debug" CACHE STRING "" FORCE)
	endif ()

	if (EXISTS ${FESAPI_LIBRARY_RELEASE} AND EXISTS ${FESAPI_LIBRARY_DEBUG}) 
		target_link_libraries (${PROJECT_NAME} PRIVATE optimized ${FESAPI_LIBRARY_RELEASE} debug ${FESAPI_LIBRARY_DEBUG})
	elseif (EXISTS ${FESAPI_LIBRARY_RELEASE})
		target_link_libraries (${PROJECT_NAME} PRIVATE ${FESAPI_LIBRARY_RELEASE})
	elseif (EXISTS ${FESAPI_LIBRARY_DEBUG})
		target_link_libraries (${PROJECT_NAME} PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		message(ERROR "No FESAPI library has been set.")
	endif ()
	
elseif (UNIX)

	if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
    endif()
	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(${PROJECT_NAME} PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		target_link_libraries(${PROJECT_NAME} PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
	
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
	)
	
endif (WIN32)
	
if (EXISTS ${AVRO_LIBRARY_RELEASE})
	target_link_libraries (${PROJECT_NAME} PRIVATE ${AVRO_LIBRARY_RELEASE})
endif()

# ============================================================================
# Win32 Visual Studio file hierarchy
# ============================================================================

if (WIN32)
	source_group ("VTK" FILES ${FESPP_VTK_SOURCES} ${FESPP_VTK_HEADERS})
	source_group ("QT" FILES ${FESPP_QT_SOURCES})
	IF (WITH_ETP)
		source_group ("etp" FILES ${FESPP_ETP_SOURCES} ${FESPP_ETP_HEADERS})
	ENDIF (WITH_ETP)
	
	add_custom_command(
		TARGET Fespp 
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			$<$<CONFIG:Debug>:$<TARGET_FILE:Fespp>> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:Fespp>>
			$<$<CONFIG:RelWithDebInfo>:$<TARGET_FILE:Fespp>> $<$<CONFIG:RelWithDebInfo>:$<TARGET_PDB_FILE:Fespp>>
			$<$<CONFIG:Debug>:"C:/pv56/install_x64_vs2017/bin"> $<$<CONFIG:RelWithDebInfo>:"C:/pv56/install_x64_vs2017/bin">
	)
endif (WIN32)

# ============================================================================
# Installation
# ============================================================================

install (
	TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
)
