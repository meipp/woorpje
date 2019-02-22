add_library (reachability OBJECT solver.cpp)
target_include_directories(reachability PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 	PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
									 )
