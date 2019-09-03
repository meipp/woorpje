include(ExternalProject)
ExternalProject_Add(smtparserr
	GIT_REPOSITORY git@gitlab.com:dannybpoulsen/smtlib-parser.git
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/SMTPARSER/install/
)

	add_library(smtparser_imp SHARED IMPORTED)	
	add_dependencies(smtparser_imp smtparserr)
 	set_property(TARGET smtparser_imp PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/SMTPARSER/install/lib/libsmtparser.a)
	

	SET(SMTPARSER_LIBRARIES  smtparser_imp)
	SET(SMTPARSER_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/SMTPARSER/install/include)
	install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/SMTPARSER/install/lib DESTINATION .)
