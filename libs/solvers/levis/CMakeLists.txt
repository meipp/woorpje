option (ENABLE_GRAPHLEVIS "Enable Graphs From Levis")


add_library (levis solver.cpp heuristics.cpp passed.cpp)
target_include_directories(levis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../pubinclude
									 	PUBLIC $<TARGET_PROPERTY:words,INTERFACE_INCLUDE_DIRECTORIES>
									 	PUBLIC $<TARGET_PROPERTY:smtsolver,INTERFACE_INCLUDE_DIRECTORIES>
)


if (ENABLE_GRAPHLEVIS)
   find_package(CGraph)
   add_definitions (-DENABLEGRAPH)	
   target_sources (levis PRIVATE graph.cpp)
   target_link_libraries (levis INTERFACE ${CGRAPH_LIBRARIES})
endif(ENABLE_GRAPHLEVIS)


