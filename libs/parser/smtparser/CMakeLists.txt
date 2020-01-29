cmake_minimum_required (VERSION 3.1)




set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-field-initializers")


add_library (sparser parser.cpp)
target_include_directories(sparser
								     PRIVATE ${SMTPARSER_INCLUDE_DIRS} 
									 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
									 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 PUBLIC $<TARGET_PROPERTY:glucose,INTERFACE_INCLUDE_DIRECTORIES>									 
									 PRIVATE $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
									 PRIVATE $<TARGET_PROPERTY:solvers,INTERFACE_INCLUDE_DIRECTORIES>
							)
target_link_libraries (sparser smtparser)
target_compile_options (sparser PRIVATE -Wall -Wpedantic -Wextra -fPIC)
