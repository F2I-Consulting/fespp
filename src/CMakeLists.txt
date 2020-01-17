#--------------------------------------------------
set (PLUGIN_NAME Fespp)

# version mechanism
set (PLUGIN_VERSION_MAJOR 2)
set (PLUGIN_VERSION_MINOR 1)
set (PLUGIN_VERSION_PATCH 0)
set (PLUGIN_VERSION ${Fesapi_VERSION_MAJOR}.${Fesapi_VERSION_MINOR}.${Fesapi_VERSION_PATCH})

#--------------------------------------------------
set (FESPP_ROOT_DIR ${CMAKE_SOURCE_DIR})

FILE (GLOB FESPP_READER_SOURCES ${FESPP_ROOT_DIR}/src/Fespp.cxx)
FILE (GLOB FESPP_READER_HEADERS ${FESPP_ROOT_DIR}/src/Fespp.h)
FILE (GLOB FESPP_READER_XML ${FESPP_ROOT_DIR}/src/Fespp.xml)
FILE (GLOB FESPP_VTK_SOURCES ${FESPP_ROOT_DIR}/src/VTK/*.cxx)
FILE (GLOB FESPP_VTK_HEADERS ${FESPP_ROOT_DIR}/src/VTK/*.h)
FILE (GLOB FESPP_PQ_SOURCES ${FESPP_ROOT_DIR}/src/PQ*.cxx)
FILE (GLOB FESPP_PQ_HEADERS ${FESPP_ROOT_DIR}/src/PQ*.h)
FILE (GLOB FESPP_FORMS ${FESPP_ROOT_DIR}/src/Resources/*.ui)
FILE (GLOB FESPP_RESOURCES ${FESPP_ROOT_DIR}/src/Resources/*.qrc)

IF (WITH_ETP)
	FILE (GLOB FESPP_ETP_SOURCES ${FESPP_ROOT_DIR}/src/etp/*.cxx)
	FILE (GLOB FESPP_ETP_HEADERS ${FESPP_ROOT_DIR}/src/etp/*.h)
ELSE(WITH_ETP)
	FILE (GLOB FESPP_PQETP_SOURCES ${FESPP_ROOT_DIR}/src/PQEtp*.cxx)
	FILE (GLOB FESPP_PQETP_HEADERS ${FESPP_ROOT_DIR}/src/PQEtp*.h)
	FILE (GLOB FESPP_FORMSETP ${FESPP_ROOT_DIR}/src/Resources/PQEtp*.ui)
	LIST (REMOVE_ITEM FESPP_PQ_SOURCES ${FESPP_PQETP_SOURCES})
	LIST (REMOVE_ITEM FESPP_PQ_HEADERS ${FESPP_PQETP_HEADERS})
	LIST (REMOVE_ITEM FESPP_FORMS ${FESPP_FORMSETP})
ENDIF (WITH_ETP)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${FESPP_ROOT_DIR}/cmake/modules)

pv_process_modules()

IF(PARAVIEW_BUILD_QT_GUI)
	IF(PARAVIEW_QT_VERSION VERSION_GREATER "4")
		set (Qt5_FIND_COMPONENTS Widgets)
#		INCLUDE(ParaViewQt5)
		QT5_WRAP_CPP(MOC_SRCS ${FESPP_PQ_HEADERS})					
		QT5_WRAP_UI(UI_SRCS ${FESPP_FORMS})
	ELSE()
		INCLUDE(${QT_USE_FILE})
		QT4_WRAP_CPP(MOC_SRCS ${FESPP_PQ_HEADERS})				
		QT4_WRAP_UI(UI_SRCS ${FESPP_FORMS})
	ENDIF()

	ADD_PARAVIEW_DOCK_WINDOW(
		IFACES
		IFACE_SRCS
		CLASS_NAME PQSelectionPanel)
		
	ADD_PARAVIEW_DOCK_WINDOW(
		IFACES2
		IFACE2_SRCS
		CLASS_NAME PQMetaDataPanel)

	IF (WITH_ETP)
		ADD_PARAVIEW_DOCK_WINDOW(
			IFACES4
			IFACE4_SRCS
			CLASS_NAME PQEtpPanel)
	ENDIF (WITH_ETP)
		
	ADD_PARAVIEW_ACTION_GROUP(
		IFACES3
		IFACE3_SRCS
		CLASS_NAME PQToolsActionGroup
		GROUP_NAME "ToolBar/EPC")

  # create a plugin for this panel
	IF (WITH_ETP)
		ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
			DOCUMENTATION_DIR "${FESPP_ROOT_DIR}/doc"
			SERVER_MANAGER_SOURCES  ${FESPP_READER_SOURCES}
			SERVER_MANAGER_XML ${FESPP_READER_XML}
        	GUI_INTERFACES ${IFACES} ${IFACES2} ${IFACES3} ${IFACES4}
			GUI_RESOURCES ${FESPP_RESOURCES}
        	SOURCES ${FESPP_PQ_SOURCES}		
					${MOC_SRCS} 
					${UI_SRCS} 
					${IFACE_SRCS} 
					${IFACE2_SRCS} 
					${IFACE3_SRCS}
					${IFACE4_SRCS}
					${FESPP_VTK_SOURCES}
					${FESPP_VTK_HEADERS}
					${FESPP_ETP_SOURCES}
					${FESPP_ETP_HEADERS}
		)
	ELSE (WITH_ETP)
		ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
			DOCUMENTATION_DIR "${FESPP_ROOT_DIR}/doc"
			SERVER_MANAGER_SOURCES  ${FESPP_READER_SOURCES}
			SERVER_MANAGER_XML ${FESPP_READER_XML}
        	GUI_INTERFACES ${IFACES} ${IFACES2} ${IFACES3}
			GUI_RESOURCES ${FESPP_RESOURCES}
        	SOURCES ${FESPP_PQ_SOURCES}		
					${MOC_SRCS} 
					${UI_SRCS} 
					${IFACE_SRCS} 
					${IFACE2_SRCS} 
					${IFACE3_SRCS}
					${IFACE4_SRCS}
					${FESPP_VTK_SOURCES}
					${FESPP_VTK_HEADERS}
		)	
	ENDIF (WITH_ETP)

ELSE(PARAVIEW_BUILD_QT_GUI)

	# create a Server plugin
	IF (WITH_ETP)
		ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
			SERVER_MANAGER_SOURCES	${FESPP_READER_SOURCES}
			SERVER_MANAGER_XML ${FESPP_READER_XML}
			REQUIRED_ON_SERVER
			SOURCES ${FESPP_VTK_SOURCES}
					${FESPP_VTK_HEADERS}
					${FESPP_ETP_SOURCES}
					${FESPP_ETP_HEADERS}
		)
	ELSE (WITH_ETP)
		ADD_PARAVIEW_PLUGIN(${PLUGIN_NAME} ${PLUGIN_VERSION}
			SERVER_MANAGER_SOURCES	${FESPP_READER_SOURCES}
			SERVER_MANAGER_XML ${FESPP_READER_XML}
			REQUIRED_ON_SERVER
			SOURCES ${FESPP_VTK_SOURCES}
					${FESPP_VTK_HEADERS}
		)	
	ENDIF (WITH_ETP)

ENDIF(PARAVIEW_BUILD_QT_GUI)

# ============================================================================
# Header files
# ============================================================================

target_include_directories(${PLUGIN_NAME} PRIVATE ${FESAPI_INCLUDE})

if (WITH_ETP)
	target_include_directories(${PLUGIN_NAME} PRIVATE ${AVRO_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
endif (WITH_ETP)

# ============================================================================
# compilation options
# ============================================================================

target_compile_options(${PLUGIN_NAME} PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/MP>
)

# ============================================================================
# Linker
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
		target_link_libraries (${PLUGIN_NAME} PRIVATE optimized ${FESAPI_LIBRARY_RELEASE} debug ${FESAPI_LIBRARY_DEBUG})
	elseif (EXISTS ${FESAPI_LIBRARY_RELEASE})
		target_link_libraries (${PLUGIN_NAME} PRIVATE ${FESAPI_LIBRARY_RELEASE})
	elseif (EXISTS ${FESAPI_LIBRARY_DEBUG})
		target_link_libraries (${PLUGIN_NAME} PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		message(ERROR "No FESAPI library has been set.")
	endif ()
	
elseif (UNIX)

	if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE RelWithDebInfo)
    endif()
	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_link_libraries(${PLUGIN_NAME} PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		target_link_libraries(${PLUGIN_NAME} PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
	
	set_target_properties(${PLUGIN_NAME} PROPERTIES
		VERSION ${PLUGIN_VERSION}
		SOVERSION ${PLUGIN_VERSION_MAJOR}.${PLUGIN_VERSION_MINOR}
	)
	
endif (WIN32)

# ============================================================================
# Installation
# ============================================================================

install (
	TARGETS ${PLUGIN_NAME}
	DESTINATION lib
)
