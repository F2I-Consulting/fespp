# The following are set after configuration is done: 
#  FESAPI_DIR

IF(NOT FESAPI_FOUND)

	find_path(FESAPI_INCLUDE NAMES ${FESAPI_DIR}/include/common/EpcDocument.h
 		PATHS	${FESAPI_DIR}/include/)
	
	find_library(FESAPI_LIBRARY NAMES ${FESAPI_DIR}/lib/libFesapiCppUnderDev.so 
 		PATHS	${FESAPI_DIR}/lib/)

	mark_as_advanced(FESAPI_INCLUDE FESAPI_LIBRARY)

	Set(FESAPI_FOUND "NO")
	IF(FESAPI_LIBRARY AND FESAPI_INCLUDE)
		Set(FESAPI_FOUND "YES")
		link_directories(${FESAPI_LIBRARY})
		ADD_DEFINITIONS(-DOSGEARTH_HAVE_FESAPI)
	ENDIF(FESAPI_LIBRARY AND FESAPI_INCLUDE)
ENDIF(NOT FESAPI_FOUND)
