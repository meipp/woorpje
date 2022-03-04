FIND_PATH(LIBMURMUR_INCLUDE_DIR NAMES murmurhash.h)
FIND_LIBRARY(LIBMURMUR_LIBRARY NAMES libmurmurhash.a murmurhash)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Murmur DEFAULT_MSG LIBMURMUR_LIBRARY LIBMURMUR_INCLUDE_DIR )

IF(Murmur_FOUND)
	SET(MURMUR_LIBRARIES ${LIBMURMUR_LIBRARY} )
	SET(MURMUR_INCLUDE_DIRS ${LIBMURMUR_INCLUDE_DIR})
	install (FILES ${MURMUR_LIBRARIES} DESTINATION lib)
ELSE(Murmur_FOUND)
	include(ExternalProject)
	ExternalProject_Add(murmurhash
  	URL "https://github.com/kloetzl/libmurmurhash/releases/download/v1.5/libmurmurhash-1.5.tar.gz"
	PREFIX ${CMAKE_CURRENT_BINARY_DIR}/murmurhash
	BUILD_IN_SOURCE 1
	CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/
	BUILD_COMMAND make
	INSTALL_COMMAND make install
	)

	add_library(murmur SHARED IMPORTED)	
	add_dependencies(murmur murmurhash)
	set_property(TARGET murmur PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/lib/libmurmurhash.a)

	SET(MURMUR_LIBRARIES  murmur)
	SET(MURMUR_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/include)
	install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/murmurhash/install/lib DESTINATION .)
ENDIF(Murmur_FOUND)
