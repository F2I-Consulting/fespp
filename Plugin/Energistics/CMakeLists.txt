set(classes
  vtkEPCReader
  vtkETPSource
)

FILE (GLOB FESPP_MAPPING_SOURCES Mapping/*.cxx)
FILE (GLOB FESPP_MAPPING_HEADERS Mapping/*.h Tools/*.h)
set (ENERGISTICS_SOURCES
	${FESPP_MAPPING_SOURCES}
)
set (ENERGISTICS_PRIVATE_HEADERS
	${FESPP_MAPPING_HEADERS}
)

vtk_module_add_module(Energistics
  CLASSES ${classes} # shortcut for adding `<class>.cxx` to `SOURCES` and `<class>.h` to `HEADERS`
  SOURCES ${ENERGISTICS_SOURCES} # Do not wrap SOURCES
  PRIVATE_HEADERS ${ENERGISTICS_PRIVATE_HEADERS} # Do not wrap PRIVATE_HEADERS but only HEADERS (or CLASSES which also defines HEADERS)
)

if (WITH_ETP_SSL)
	vtk_module_definitions(Energistics PRIVATE
		WITH_ETP_SSL
		BOOST_ALL_NO_LIB
	)
	if (NOT Boost_USE_STATIC_LIBS)
		# https://www.boost.org/doc/libs/1_75_0/libs/log/doc/html/log/rationale/namespace_mangling.html
		vtk_module_definitions(Energistics PRIVATE BOOST_LOG_DYN_LINK)
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
		vtk_module_definitions(Energistics PRIVATE _WIN32_WINNT=${ver})
	endif (WIN32)
endif (WITH_ETP_SSL)

# ============================================================================
# External include
# ============================================================================
vtk_module_include(Energistics PRIVATE ${FESAPI_INCLUDE_DIR})
if (WITH_ETP_SSL)
	vtk_module_include(Energistics PRIVATE ${FETPAPI_INCLUDE_DIR} ${AVRO_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR})
endif (WITH_ETP_SSL)

# ============================================================================
# External link
# ============================================================================

if (WIN32)
	if (EXISTS ${FESAPI_LIBRARY_RELEASE})
		vtk_module_link(Energistics PRIVATE ${FESAPI_LIBRARY_RELEASE})
	elseif (EXISTS ${FESAPI_LIBRARY_DEBUG})
		vtk_module_link(Energistics PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		message(ERROR "No FESAPI library has been set.")
	endif ()
	
	if (WITH_ETP_SSL)
		vtk_module_link (Energistics PRIVATE bcrypt.lib)
	endif (WITH_ETP_SSL)
elseif (UNIX)	
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		vtk_module_link(Energistics PRIVATE ${FESAPI_LIBRARY_DEBUG})
	else ()
		vtk_module_link(Energistics PRIVATE ${FESAPI_LIBRARY_RELEASE})
	endif()
endif (WIN32)

if (WITH_ETP_SSL)
	vtk_module_link (Energistics PRIVATE OpenSSL::SSL OpenSSL::Crypto)
	vtk_module_link (Energistics PRIVATE ${FETPAPI_LIBRARY_RELEASE} ${AVRO_LIBRARY_RELEASE} ${Boost_SYSTEM_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
endif(WITH_ETP_SSL)

# ============================================================================
# PARAVIEW SERVER MANAGER XML
# ============================================================================

paraview_add_server_manager_xmls(
  XMLS  Energistics.xml)
  
if (DEV_UNDER_WIN32)
	add_custom_command(
		TARGET Energistics 
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			$<$<CONFIG:Debug>:$<TARGET_FILE:Energistics>> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:Energistics>>
			$<$<CONFIG:RelWithDebInfo>:$<TARGET_FILE:Energistics>> $<$<CONFIG:RelWithDebInfo>:$<TARGET_PDB_FILE:Energistics>>
			$<$<CONFIG:Debug>:"${ParaView_DIR}/bin/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}/plugins/Fespp"> $<$<CONFIG:RelWithDebInfo>:"${ParaView_DIR}/bin/paraview-${ParaView_VERSION_MAJOR}.${ParaView_VERSION_MINOR}/plugins/Fespp">
	)
endif (DEV_UNDER_WIN32)
