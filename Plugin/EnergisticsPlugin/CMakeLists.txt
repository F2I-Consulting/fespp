set(classes
  EnergisticsPlugin)

FILE (GLOB FESPP_MAPPING_SOURCES ResqmlMapping/*.cxx)
FILE (GLOB FESPP_MAPPING_HEADERS ResqmlMapping/*.h)
if (WITH_ETP)
	FILE (GLOB FESPP_ETP_SOURCES etp/*.cxx)
	FILE (GLOB FESPP_ETP_HEADERS etp/*.h)
ENDIF (WITH_ETP)
set (ENERGISTICS_READER_SOURCES
		${FESPP_MAPPING_SOURCES}
		${FESPP_ETP_SOURCES}
	)
set (ENERGISTICS_READER_PRIVATE_HEADERS
		${FESPP_MAPPING_HEADERS}
		${FESPP_ETP_HEADERS}
	)

vtk_module_add_module(EnergisticsPlugin
  CLASSES ${classes} # shortcut for adding `<class>.cxx` to `SOURCES` and `<class>.h` to `HEADERS`
  SOURCES ${ENERGISTICS_READER_SOURCES} # Do not wrap SOURCES
  PRIVATE_HEADERS ${ENERGISTICS_READER_PRIVATE_HEADERS} # Do not wrap PRIVATE_HEADERS but only HEADERS (or CLASSES which also defines HEADERS)
)

if (WITH_ETP)
	vtk_module_definitions(EnergisticsPlugin PRIVATE
		WITH_ETP
		BOOST_ALL_NO_LIB
	)
endif (WITH_ETP)

# ============================================================================
# External include
# ============================================================================
message (STATUS "test: ${FESAPI_INCLUDE}")
vtk_module_include(EnergisticsPlugin PRIVATE ${FESAPI_INCLUDE})
if (WITH_ETP)
	vtk_module_include(EnergisticsPlugin PRIVATE ${AVRO_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
endif (WITH_ETP)

# ============================================================================
# External link
# ============================================================================

if (WIN32)
	if (EXISTS ${FESAPI_LIBRARY_RELEASE})
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_RELEASE})
	elseif (EXISTS ${FESAPI_LIBRARY_DEBUG})
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		message(ERROR "No FESAPI library has been set.")
	endif ()
elseif (UNIX)	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
endif (WIN32)
	
if (EXISTS ${AVRO_LIBRARY_RELEASE})
	vtk_module_link (EnergisticsPlugin PRIVATE ${AVRO_LIBRARY_RELEASE})
endif()

# ============================================================================
# PARAVIEW SERVER MANAGER XML
# ============================================================================

paraview_add_server_manager_xmls(
  XMLS  EnergisticsPlugin.xml)
  
if (DEV_UNDER_WIN32)
	add_custom_command(
		TARGET EnergisticsPlugin 
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			$<$<CONFIG:Debug>:$<TARGET_FILE:EnergisticsPlugin>> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:EnergisticsPlugin>>
			$<$<CONFIG:RelWithDebInfo>:$<TARGET_FILE:EnergisticsPlugin>> $<$<CONFIG:RelWithDebInfo>:$<TARGET_PDB_FILE:EnergisticsPlugin>>
			$<$<CONFIG:Debug>:"C:/Dev/build/pv5.10.0/install/bin/paraview-5.10/plugins/Fespp"> $<$<CONFIG:RelWithDebInfo>:"C:/Dev/build/pv5.10.0/install/bin/paraview-5.10/plugins/Fespp">
	)
endif (DEV_UNDER_WIN32)
