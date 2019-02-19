add_library (satsolver OBJECT solver.cpp Main.cc)
target_include_directories(satsolver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
																			 )
target_link_libraries (satsolver glucose)
