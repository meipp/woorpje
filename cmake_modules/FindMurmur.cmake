FIND_PATH(LIBMURMUR_INCLUDE_DIR NAMES murmurhash.h)
FIND_LIBRARY(LIBMURMUR_LIBRARY NAMES murmurhash )


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBMURMUR DEFAULT_MSG LIBMURMUR_LIBRARY LIBMURMUR_INCLUDE_DIR )

IF(LIBMURMUR_FOUND)
	SET(MURMUR_LIBRARIES ${LIBMURMUR_LIBRARY} )
	SET(MURMUR_INCLUDE_DIRS ${LIBMURMUR_INCLUDE_DIR})
ELSE(LIBMURMUR_FOUND)
	include(ExternalProject)
	ExternalProject_Add(murmurhash
  	URL "https://github.com/kloetzl/libmurmurhash/releases/download/v1.5/libmurmurhash-1.5.tar.gz"
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/murmurhash
	BUILD_IN_SOURCE 1
	CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/
	BUILD_COMMAND make
	INSTALL_COMMAND make install
	)

	add_library(murmur STATIC IMPORTED)	
	add_dependencies(murmur murmurhash)
	set_property(TARGET murmur PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/lib/libmurmurhash.so)
	
	SET(MURMUR_LIBRARIES  murmur)
	SET(MURMUR_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/include)
ENDIF(LIBMURMUR_FOUND)