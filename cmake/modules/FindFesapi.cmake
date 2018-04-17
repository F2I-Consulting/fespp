# The following are set after configuration is done: 
#  FESAPI_DIR

IF(NOT FESAPI_FOUND)

	find_path(FESAPI_INCLUDE NAMES common/EpcDocument.h
 		PATHS	${FESAPI_DIR}/include/)

	find_library(FESAPI_LIBRARY NAMES FesapiCpp.0.11.1.1 FesapiCpp
 		PATHS	${FESAPI_DIR}/lib/)
	if(NOT FESAPI_LIBRARY)
		message(FATAL_ERROR "FESAPI library not found")
	endif()

	mark_as_advanced(FESAPI_INCLUDE FESAPI_LIBRARY)

	Set(FESAPI_FOUND "NO")
	IF(FESAPI_LIBRARY AND FESAPI_INCLUDE)
		Set(FESAPI_FOUND "YES")
		link_directories(${FESAPI_LIBRARY})
		ADD_DEFINITIONS(-DOSGEARTH_HAVE_FESAPI)
	ENDIF(FESAPI_LIBRARY AND FESAPI_INCLUDE)
ENDIF(NOT FESAPI_FOUND)
