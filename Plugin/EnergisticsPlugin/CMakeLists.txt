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
	if (NOT Boost_USE_STATIC_LIBS)
		# https://www.boost.org/doc/libs/1_75_0/libs/log/doc/html/log/rationale/namespace_mangling.html
		vtk_module_definitions(EnergisticsPlugin PRIVATE BOOST_LOG_DYN_LINK)
	endif ()
	
	if (WIN32)
		#FROM https://stackoverflow.com/a/40217291
		macro(get_WIN32_WINNT version)
			if(CMAKE_SYSTEM_VERSION)
				set(ver ${CMAKE_SYSTEM_VERSION})
				string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
				string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
				# Check for Windows 10, b/c we'll need to convert to hex 'A'.
				if("${verMajor}" MATCHES "10")
					set(verMajor "A")
					string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
				endif()
				# Remove all remaining '.' characters.
				string(REPLACE "." "" ver ${ver})
				# Prepend each digit with a zero.
				string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
				set(${version} "0x${ver}")
			endif()
		endmacro()

		get_WIN32_WINNT(ver)
		vtk_module_definitions(EnergisticsPlugin PRIVATE _WIN32_WINNT=${ver})
	endif (WIN32)
endif (WITH_ETP)

# ============================================================================
# External include
# ============================================================================
vtk_module_include(EnergisticsPlugin PRIVATE ${FESAPI_INCLUDE_DIR})
if (WITH_ETP)
	vtk_module_include(EnergisticsPlugin PRIVATE ${FETPAPI_INCLUDE_DIR} ${AVRO_INCLUDE_DIR} ${Boost_INCLUDE_DIR})
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
	
	if (WITH_ETP)
		vtk_module_link (EnergisticsPlugin PRIVATE bcrypt.lib)
	endif (WITH_ETP)
elseif (UNIX)	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		vtk_module_link(EnergisticsPlugin PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
endif (WIN32)
	
if (EXISTS ${FETPAPI_LIBRARY_RELEASE})
	vtk_module_link (EnergisticsPlugin PRIVATE ${FETPAPI_LIBRARY_RELEASE} ${AVRO_LIBRARY_RELEASE} ${Boost_SYSTEM_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
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
