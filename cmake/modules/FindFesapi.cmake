# Locate Fesapi
#
# This module defines the following CMake variables:
#
#    FESAPI_FOUND - True if Fesapi is found
#    FESAPI_LIBRARY - A variable pointing to the Fesapi library
#    FESAPI_INCLUDE_DIRS - Where to find the headers

IF(NOT FESAPI_FOUND)
	find_path(FESAPI_INCLUDE NAMES EpcDocument.h
    		PATHS 	$ENV{FESAPI_DIR}/include
			${FESAPI_DIR}/include)
	find_path(FESAPI_RESQML2_INCLUDE NAMES AbstractObject.h
		PATHS	$ENV{FESAPI_DIR}/include/resqml2
			${FESAPI_DIR}/include/resqml2)
	find_path(FESAPI_RESQML2_0_1_INCLUDE NAMES Fault.h
		PATHS	$ENV{FESAPI_DIR}/include/resqml2_0_1
			${FESAPI_DIR}/include/resqml2_0_1)
	find_path(FESAPI_EPC_INCLUDE NAMES Package.h
    		PATHS	$ENV{FESAPI_DIR}/include/epc
			${FESAPI_DIR}/include/epc)
	find_path(FESAPI_PROXIES_INCLUDE NAMES envH.h
		PATHS	$ENV{FESAPI_DIR}/include/proxies
			${FESAPI_DIR}/include/proxies)
	find_path(FESAPI_TOOLS_INCLUDE NAMES GuidTools.h
    		PATHS	$ENV{FESAPI_DIR}/include/tools
			${FESAPI_DIR}/include/tools)
	find_path(FESAPI_WITSML1_4_1_1_INCLUDE NAMES AbstractObject.h
	    	PATHS	$ENV{FESAPI_DIR}/include/witsml1_4_1_1
			${FESAPI_DIR}/include/witsml1_4_1_1)

	find_library(FESAPI_LIBRARY NAMES FesapiCpp
		PATHS	$ENV{FESAPI_DIR}/lib
			${FESAPI_DIR}/lib)

	set(FESAPI_INCLUDE_DIRS
  		${FESAPI_INCLUDE} 
		${FESAPI_RESQML2_INCLUDE} 
		${FESAPI_RESQML2_0_1_INCLUDE} 
		${FESAPI_EPC_INCLUDE} 
		${FESAPI_PROXIES_INCLUDE}
  		${FESAPI_TOOLS_INCLUDE} 
		${FESAPI_WITSML1_4_1_1_INCLUDE} 
	)

	Set(FESAPI_FOUND "NO")
	IF(FESAPI_LIBRARY AND FESAPI_INCLUDE_DIRS)
		Set(FESAPI_FOUND "YES")
		link_directories(${FESAPI_LIBRARY})
		ADD_DEFINITIONS(-DOSGEARTH_HAVE_FESAPI)
	ENDIF(FESAPI_LIBRARY AND FESAPI_INCLUDE_DIRS)
ENDIF(NOT FESAPI_FOUND)
