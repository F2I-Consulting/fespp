set(classes
  vtkEPCReader)

FILE (GLOB FESPP_VTK_SOURCES VTK/*.cxx)
FILE (GLOB FESPP_VTK_HEADERS VTK/*.h)
if (WITH_ETP)
	FILE (GLOB FESPP_ETP_SOURCES etp/*.cxx)
	FILE (GLOB FESPP_ETP_HEADERS etp/*.h)
ENDIF (WITH_ETP)
set (EPC_READER_SOURCES
		${FESPP_VTK_SOURCES}
		${FESPP_ETP_SOURCES}
	)
set (EPC_READER_PRIVATE_HEADERS
		${FESPP_VTK_HEADERS}
		${FESPP_ETP_HEADERS}
	)

vtk_module_add_module(EPCReader
  CLASSES ${classes} # shortcut for adding `<class>.cxx` to `SOURCES` and `<class>.h` to `HEADERS`
  SOURCES ${EPC_READER_SOURCES} # Do not wrap SOURCES
  PRIVATE_HEADERS ${EPC_READER_PRIVATE_HEADERS} # Do not wrap PRIVATE_HEADERS but only HEADERS (or CLASSES which also defines HEADERS)
)

if (WITH_ETP)
	vtk_module_definitions(EPCReader PRIVATE
		WITH_ETP
		BOOST_ALL_NO_LIB
	)
endif (WITH_ETP)

# ============================================================================
# External include
# ============================================================================

vtk_module_include(EPCReader PRIVATE ${FESAPI_INCLUDE})
if (WITH_ETP)
	vtk_module_include(EPCReader PRIVATE ${AVRO_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
endif (WITH_ETP)

# ============================================================================
# External link
# ============================================================================

if (WIN32)
	if (EXISTS ${FESAPI_LIBRARY_RELEASE})
		vtk_module_link(EPCReader PRIVATE ${FESAPI_LIBRARY_RELEASE})
	elseif (EXISTS ${FESAPI_LIBRARY_DEBUG})
		vtk_module_link(EPCReader PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		message(ERROR "No FESAPI library has been set.")
	endif ()
elseif (UNIX)	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		vtk_module_link(EPCReader PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		vtk_module_link(EPCReader PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
endif (WIN32)
	
if (EXISTS ${AVRO_LIBRARY_RELEASE})
	vtk_module_link (EPCReader PRIVATE ${AVRO_LIBRARY_RELEASE})
endif()

# ============================================================================
# PARAVIEW SERVER MANAGER XML
# ============================================================================

paraview_add_server_manager_xmls(
  XMLS  EPCReader.xml)
  
if (DEV_UNDER_WIN32)
	add_custom_command(
		TARGET EPCReader 
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			$<$<CONFIG:Debug>:$<TARGET_FILE:EPCReader>> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:EPCReader>>
			$<$<CONFIG:RelWithDebInfo>:$<TARGET_FILE:EPCReader>> $<$<CONFIG:RelWithDebInfo>:$<TARGET_PDB_FILE:EPCReader>>
			$<$<CONFIG:Debug>:"C:/pv57/pvbd/bin/paraview-5.7/plugins/Fespp"> $<$<CONFIG:RelWithDebInfo>:"C:/pv57/install_x64_vs2017/bin/paraview-5.7/plugins/Fespp">
	)
endif (DEV_UNDER_WIN32)
