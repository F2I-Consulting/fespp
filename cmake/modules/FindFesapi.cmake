# The following are set after configuration is done: 
#  FESAPI_DIR

if(NOT FESAPI_FOUND)

	find_path(FESAPI_INCLUDE NAMES ${FESAPI_DIR}/include/common/EpcDocument.h
 		PATHS	${FESAPI_DIR}/include/)
	
	find_library(FESAPI_LIBRARY_RELEASE NAMES ${FESAPI_DIR}/lib/libFesapiCpp.1.0.0.0.so 
 		PATHS	${FESAPI_DIR}/lib/)
		
	find_library(FESAPI_LIBRARY_DEBUG NAMES ${FESAPI_DIR}/lib/libFesapiCppd.1.0.0.0.so 
 		PATHS	${FESAPI_DIR}/lib/)

	mark_as_advanced(FESAPI_INCLUDE FESAPI_LIBRARY_RELEASE FESAPI_LIBRARY_DEBUG)

	set(FESAPI_FOUND "NO")
	if((FESAPI_LIBRARY_RELEASE OR FESAPI_LIBRARY_DEBUG) AND FESAPI_INCLUDE)
		set(FESAPI_FOUND "YES")
		add_definitions(-DOSGEARTH_HAVE_FESAPI)
	endif((FESAPI_LIBRARY_RELEASE OR FESAPI_LIBRARY_DEBUG) AND FESAPI_INCLUDE)
endif(NOT FESAPI_FOUND)
